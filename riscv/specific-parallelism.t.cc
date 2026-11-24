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

bool test_bitmanip(machine_t& m)
{
  constexpr uint32_t regs = (1u << 15) | (2u << 20) | (3u << 7);
  m.state.XPR.write(1, 0x00ff55aa);
  m.state.XPR.write(2, 0x00005500);
  if (!check(m.step(MATCH_ANDN | regs) && m.state.XPR[3] == 0x00ff00aa, "check ANDN"))
    return false;

  m.state.XPR.write(1, 0x55001122);
  m.state.XPR.write(2, 1);
  const bool rv64 = m.proc.get_xlen() == 64;
  const reg_t perm16 = rv64 ? 0x1122112211225500ULL : 0x11225500;
  if (!check(m.step(MATCH_XPERM16 | regs) && m.state.XPR[3] == perm16, "check XPERM16"))
    return false;
  if (!rv64)
    return true;

  m.state.XPR.write(1, 0xabcd0123456789efULL);
  m.state.XPR.write(2, 0x0000000200000001ULL);
  return check(m.step(MATCH_XPERM32 | regs) && m.state.XPR[3] == 0xabcd0123, "check XPERM32");
}

bool test_user_interrupt(machine_t& m)
{
  constexpr reg_t handler = base + 0x200;
  m.proc.set_privilege(PRV_U, false);
  m.state.utvec->write(handler);
  m.state.ustatus->write(USTATUS_UIE);
  m.state.uie->write(MIP_USIP);
  m.state.uip->write(MIP_USIP);
  m.proc.step(1);

  const reg_t interrupt_bit = reg_t(1) << (m.proc.get_xlen() - 1);
  if (!check(m.state.pc == handler && m.state.uepc->read() == base, "enter the user interrupt handler") ||
      !check((m.state.ucause->read() & interrupt_bit) && m.state.ustatus->read() == USTATUS_UPIE,
             "save the interrupt enable and report its cause"))
    return false;

  m.state.uip->write(0);
  return check(m.execute(MATCH_URET, base), "return from the user interrupt") &&
         check(m.state.prv == PRV_U && (m.state.ustatus->read() & USTATUS_UIE),
               "restore user mode and interrupt enable");
}

#ifdef HAVE_INT128
bool test_packed(machine_t& m)
{
  m.state.XPR.write(1, 0x00ff7fff);
  m.state.XPR.write(2, 0x01010101);
  constexpr uint32_t regs = (1u << 15) | (2u << 20) | (3u << 7);
  return check(m.step(MATCH_PADD_B | regs) && m.state.XPR[3] == 0x01008000,
               "discard carries between packed byte lanes");
}
#endif

bool test_transaction(machine_t& m)
{
  constexpr uint32_t begin = MATCH_TBEGIN | (5u << 7);
  constexpr uint32_t test = MATCH_TTEST | (6u << 7);
  constexpr uint32_t store = MATCH_SW | (1u << 15) | (2u << 20);
  constexpr uint32_t load = MATCH_LW | (1u << 15) | (3u << 7);
  constexpr reg_t data_offset = 0x1000;

  m.state.XPR.write(1, base + data_offset);
  m.state.XPR.write(2, 42);
  if (!check(m.step(begin) && m.state.XPR[5] == 0, "begin a transaction") ||
      !check(m.step(test) && m.state.XPR[6] == 1, "report the active transaction") ||
      !check(m.step(store) && m.step(load) && m.state.XPR[3] == 42, "read a buffered store"))
    return false;

  uint32_t memory = 0;
  if (!check(m.mem.load(data_offset, sizeof(memory), reinterpret_cast<uint8_t*>(&memory)) &&
             from_le(memory) == 0, "hide the buffered store before commit") ||
      !check(m.step(MATCH_TCOMMIT) && m.step(test) && m.state.XPR[6] == 0, "commit a transaction") ||
      !check(m.mem.load(data_offset, sizeof(memory), reinterpret_cast<uint8_t*>(&memory)) &&
             from_le(memory) == 42, "publish the committed store") ||
      !check(m.step(begin), "begin a second transaction"))
    return false;

  const reg_t resume = m.state.pc;
  m.state.XPR.write(2, 43);
  return check(m.step(store) && m.execute(MATCH_TABORT | (7u << 20), resume), "abort a transaction") &&
         check(m.state.XPR[2] == 42 && m.state.XPR[5] == ((7u << 16) | TX_EXPLICIT),
               "restore registers and report the abort code") &&
         check(m.mem.load(data_offset, sizeof(memory), reinterpret_cast<uint8_t*>(&memory)) &&
               from_le(memory) == 42, "discard the aborted store");
}

constexpr struct {
  const char* name;
  const char* isa;
  bool (*run)(machine_t&);
} tests[] = {
  {"B", "gcb_zbp", test_bitmanip},
  {"N", "gc_xn", test_user_interrupt},
#ifdef HAVE_INT128
  {"P", "gcp", test_packed},
#endif
  {"T", "gc_xt", test_transaction},
};

}

int main(int argc, char** argv)
{
  const std::string_view selected = argc == 2 ? argv[1] : "default";
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [B|N|P|T]\n", argv[0]);
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
    fprintf(stderr, "Unknown test. Select B|N|P|T.\n");
  return matched && passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
