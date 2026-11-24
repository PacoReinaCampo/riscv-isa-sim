#include "config.h"
#include "sim.h"
#include "encoding.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

namespace {

constexpr reg_t base = 0x20000;
constexpr reg_t trap_pc = base + 0x800;
constexpr unsigned mem_size = 0x2000;

cfg_t make_config(const std::string& isa)
{
  cfg_t cfg;
  cfg.isa = isa.c_str();
  cfg.mem_layout = {mem_cfg_t(base, mem_size)};
  return cfg;
}

struct machine_t {
  cfg_t cfg;
  mem_t mem{mem_size};
  sim_t sim;
  processor_t& proc;
  state_t& state;

  explicit machine_t(const std::string& isa)
    : cfg(make_config(isa)),
      sim(&cfg, false, {{base, &mem}}, {}, false, {"none"},
          debug_module_config_t{}, nullptr, true, nullptr, false, nullptr, std::nullopt),
      proc(*sim.get_core(size_t(0))), state(*proc.get_state())
  {
    state.pc = base;
    state.mtvec->write(trap_pc);
  }

  bool execute(uint32_t opcode, reg_t expected_pc)
  {
    const auto bytes = to_le(opcode);
    if (!mem.store(state.pc - base, sizeof(bytes), reinterpret_cast<const uint8_t*>(&bytes)))
      return false;
    proc.step(1);
    return state.pc == expected_pc;
  }

  bool step(uint32_t opcode)
  {
    return execute(opcode, state.pc + 4);
  }
};

bool check(bool passed, const char* operation)
{
  if (!passed)
    fprintf(stderr, "FAIL: %s\n", operation);
  return passed;
}

bool test_user(machine_t& m)
{
  m.proc.set_privilege(PRV_U, false);
  if (!check(m.step(MATCH_UFENCE_VMA), "execute UFENCE.VMA in native U mode"))
    return false;

  const reg_t pc = m.state.pc;
  return check(m.execute(MATCH_UTRAPRET, trap_pc) &&
               m.state.mcause->read() == CAUSE_ILLEGAL_INSTRUCTION &&
               m.state.mepc->read() == pc, "reject UTRAPRET without an active user trap frame");
}

bool test_supervisor(machine_t& m)
{
  constexpr reg_t resume = base + 0x200;
  m.proc.set_privilege(PRV_S, false);
  m.state.sepc->write(resume);
  const auto status = m.state.mstatus->read();
  m.state.mstatus->write((status & ~(MSTATUS_SPP | MSTATUS_SIE)) | MSTATUS_SPIE);
  return check(m.step(MATCH_SFENCE_VMA), "execute SFENCE.VMA in S mode") &&
         check(m.execute(MATCH_SRET, resume), "return to the supervisor exception PC") &&
         check(m.state.prv == PRV_U && !m.state.v, "return to native U mode") &&
         check(m.state.mstatus->read() & MSTATUS_SIE, "restore the S-mode interrupt-enable bit");
}

bool test_hypervisor(machine_t& m)
{
  m.proc.set_privilege(PRV_S, false);
  if (!check(m.step(MATCH_HFENCE_VVMA), "execute HFENCE.VVMA in HS mode") ||
      !check(m.step(MATCH_HFENCE_GVMA), "execute HFENCE.GVMA in HS mode"))
    return false;

  m.proc.set_privilege(PRV_S, true);
  return check(m.execute(MATCH_HFENCE_VVMA, trap_pc) &&
               m.state.mcause->read() == CAUSE_VIRTUAL_INSTRUCTION,
               "reject HFENCE.VVMA in VS mode with a virtual-instruction trap");
}

bool test_machine(machine_t& m)
{
  constexpr reg_t resume = base + 0x200;
  m.state.mepc->write(resume);
  const auto status = m.state.mstatus->read();
  m.state.mstatus->write((status & ~(MSTATUS_MPP | MSTATUS_MIE)) | MSTATUS_MPIE);
  return check(m.step(MATCH_MFENCE_VMA), "execute MFENCE.VMA in M mode") &&
         check(m.execute(MATCH_MTRAPRET, resume), "return to the machine exception PC") &&
         check(m.state.prv == PRV_U && !m.state.v, "restore native U privilege") &&
         check(m.state.mstatus->read() & MSTATUS_MIE, "restore the M-mode interrupt-enable bit");
}

constexpr struct {
  const char* name;
  const char* isa;
  bool (*run)(machine_t&);
} tests[] = {
  {"U", "gc_xu", test_user},
  {"S", "gc", test_supervisor},
  {"H", "gch", test_hypervisor},
  {"X", "gc_xx", test_machine},
};

}

int main(int argc, char** argv)
{
  const std::string_view selected = argc == 2 ? argv[1] : "default";
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [U|S|H|X]\n", argv[0]);
    return EXIT_FAILURE;
  }

  bool matched = false;
  bool passed = true;
  for (const auto& test : tests) {
    if (selected != "default" && selected != test.name)
      continue;
    matched = true;
    for (unsigned xlen : {32u, 64u}) {
      const std::string isa = "rv" + std::to_string(xlen) + test.isa;
      machine_t machine(isa);
      const bool ok = test.run(machine);
      printf("%s RV%u: %s\n", test.name, xlen, ok ? "passed" : "FAILED");
      passed &= ok;
    }
  }
  if (!matched)
    fprintf(stderr, "Unknown test. Select U|S|H|X.\n");
  return matched && passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
