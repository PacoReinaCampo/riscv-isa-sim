#include "sim.h"
#include "encoding.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

namespace {

constexpr reg_t base = 0x20000;
constexpr reg_t source_offset = 0x1000;
constexpr reg_t result_offset = 0x1100;

struct profile_t {
  const char* name;
  const char* isa;
  uint32_t set_vl;
  uint32_t add;
  uint32_t load;
  uint32_t store;
};

constexpr profile_t profiles[] = {
  {"V", "v", MATCH_VSETVLI, MATCH_VADD_VV,
   MATCH_VLE8_V | (1u << 29), MATCH_VSE8_V | (1u << 29)},
  {"W", "_xw", MATCH_WSETWLI, MATCH_WADD_WW, MATCH_WLSEG2E8_W, MATCH_WSSEG2E8_W},
  {"Y", "_xy", MATCH_YSETYLI, MATCH_YADD_YY, MATCH_YLSEG2E8_Y, MATCH_YSSEG2E8_Y},
};

cfg_t make_config(const std::string& isa)
{
  cfg_t cfg;
  cfg.isa = isa.c_str();
  cfg.mem_layout = {mem_cfg_t(base, 0x2000)};
  return cfg;
}

struct machine_t {
  cfg_t cfg;
  mem_t mem{0x2000};
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
    state.mstatus->write(state.mstatus->read() | MSTATUS_VS);
  }

  bool step(uint32_t opcode)
  {
    const reg_t pc = state.pc;
    const auto bytes = to_le(opcode);
    if (!mem.store(pc - base, sizeof(bytes), reinterpret_cast<const uint8_t*>(&bytes)))
      return false;
    proc.step(1);
    return state.pc == pc + sizeof(bytes);
  }

  bool set_vl(uint32_t opcode, unsigned vl)
  {
    state.XPR.write(1, vl);
    return step(opcode | (1u << 15) | (3u << 7)) &&
           state.XPR[3] == vl;
  }
};

bool check(bool passed, const char* operation)
{
  if (!passed)
    fprintf(stderr, "FAIL: %s\n", operation);
  return passed;
}

bool test_algebra(const profile_t& p, const std::string& isa)
{
  machine_t m(isa);
  constexpr unsigned unmasked = 1u << 25;
  const std::array<uint8_t, 8> input{1, 9, 2, 8, 3, 7, 4, 6};
  const std::array<uint8_t, 8> expected{10, 9, 10, 8, 10, 7, 10, 6};
  std::array<uint8_t, 8> result{};

  if (!check(m.set_vl(p.set_vl, 4), "set VL to four 8-bit elements") ||
      !check(m.mem.store(source_offset, input.size(), input.data()), "initialize input memory"))
    return false;

  m.state.XPR.write(1, base + source_offset);
  if (!check(m.step(p.load | unmasked | (1u << 15) | (8u << 7)), "load two interleaved fields") ||
      !check(m.step(p.add | unmasked | (9u << 20) | (8u << 15) | (8u << 7)), "add the fields"))
    return false;

  m.state.XPR.write(1, base + result_offset);
  return check(m.step(p.store | unmasked | (1u << 15) | (8u << 7)), "store two interleaved fields") &&
         check(m.mem.load(result_offset, result.size(), result.data()) && result == expected,
               "compare the stored values");
}

}

int main(int argc, char** argv)
{
  const std::string_view selected = argc == 2 ? argv[1] : "default";
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [V|W|Y]\n", argv[0]);
    return EXIT_FAILURE;
  }

  bool matched = false;
  bool passed = true;
  for (const auto& profile : profiles) {
    if (selected != "default" && selected != profile.name)
      continue;
    matched = true;
    for (unsigned xlen : {32u, 64u}) {
      const std::string base_isa = "rv" + std::to_string(xlen) + "gc";
      const bool ok = test_algebra(profile, base_isa + profile.isa);
      printf("%s RV%u: %s\n", profile.name, xlen, ok ? "passed" : "FAILED");
      passed &= ok;
    }
  }
  if (!matched)
    fprintf(stderr, "Unknown profile. Select V, W, or Y.\n");
  return matched && passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
