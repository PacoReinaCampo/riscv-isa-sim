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

struct test_t {
  const char* name;
  const char* isa;
  uint32_t from_int;
  uint32_t add;
  uint32_t divide;
  uint32_t to_int;
};

constexpr test_t tests[] = {
  {"F", "if", MATCH_FCVT_S_W, MATCH_FADD_S, MATCH_FDIV_S, MATCH_FCVT_W_S},
  {"D", "ifd", MATCH_FCVT_D_W, MATCH_FADD_D, MATCH_FDIV_D, MATCH_FCVT_W_D},
#ifdef HAVE_INT128
  {"Q", "ifdq", MATCH_FCVT_Q_W, MATCH_FADD_Q, MATCH_FDIV_Q, MATCH_FCVT_W_Q},
#endif
};

bool test_arithmetic(machine_t& m, const test_t& t)
{
  m.state.mstatus->write(m.state.mstatus->read() | MSTATUS_FS);
  m.state.XPR.write(1, 17);
  m.state.XPR.write(2, 25);
  return check(m.step(t.from_int | (1u << 15) | (1u << 7)), "convert 17 to floating point") &&
         check(m.step(t.from_int | (2u << 15) | (2u << 7)), "convert 25 to floating point") &&
         check(m.step(t.add | (2u << 20) | (1u << 15) | (3u << 7)), "add the operands") &&
         check(m.step(t.to_int | (3u << 15) | (4u << 7)), "convert the sum to an integer") &&
         check(m.state.XPR[4] == 42 && m.proc.get_csr(CSR_FFLAGS) == 0, "check the exact sum") &&
         check(m.step(t.from_int | (5u << 7)), "convert zero to floating point") &&
         check(m.step(t.divide | (5u << 20) | (3u << 15) | (6u << 7)), "divide the sum by zero") &&
         check(m.proc.get_csr(CSR_FFLAGS) == 8, "check the divide-by-zero flag");
}

}

int main(int argc, char** argv)
{
  const std::string_view selected = argc == 2 ? argv[1] : "default";
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [F|D|Q]\n", argv[0]);
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
      const bool ok = test_arithmetic(machine, test);
      printf("%s RV%u: %s\n", test.name, xlen, ok ? "passed" : "FAILED");
      passed &= ok;
    }
  }
  if (!matched)
    fprintf(stderr, "Unknown test. Select F|D|Q.\n");
  return matched && passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
