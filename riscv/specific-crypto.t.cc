#include "sim.h"
#include "encoding.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

namespace {

constexpr reg_t base = 0x20000;
constexpr reg_t trap_pc = base + 0x800;

struct profile_t {
  const char* name;
  const char* isa;
  const char* isa_without_crypto;
  uint32_t set_vl;
  uint32_t aes;
  size_t aes_size;
};

constexpr profile_t profiles[] = {
  {"VK", "v_zvkned", "v", MATCH_VSETVLI, MATCH_VAESZ_VS, 4},
  {"WK", "_xwk", "_xw", MATCH_WSETWLI, MATCH_WAESZ_WS, 6},
  {"YK", "_xyk", "_xy", MATCH_YSETYLI, MATCH_YAESZ_YS, 8},
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
    state.mtvec->write(trap_pc);
    state.mstatus->write(state.mstatus->read() | MSTATUS_VS);
  }

  bool execute(uint32_t opcode, size_t size, reg_t expected_pc)
  {
    const auto bytes = to_le(uint64_t(opcode));
    if (!mem.store(state.pc - base, size, reinterpret_cast<const uint8_t*>(&bytes)))
      return false;
    proc.step(1);
    return state.pc == expected_pc;
  }

  bool step(uint32_t opcode, size_t size = 4)
  {
    return execute(opcode, size, state.pc + size);
  }

  bool illegal(uint32_t opcode, size_t size)
  {
    auto pc = state.pc;
    return execute(opcode, size, trap_pc) &&
           state.mcause->read() == CAUSE_ILLEGAL_INSTRUCTION && state.mepc->read() == pc;
  }

  bool set_vl(uint32_t opcode, unsigned vl, unsigned vtype)
  {
    state.XPR.write(1, vl);
    return step(opcode | (vtype << 20) | (1u << 15) | (3u << 7)) &&
           state.XPR[3] == vl;
  }
};

bool check(bool passed, const char* operation)
{
  if (!passed)
    fprintf(stderr, "FAIL: %s\n", operation);
  return passed;
}

bool test_crypto(const profile_t& p, const std::string& base_isa)
{
  const std::string isa = base_isa + p.isa;
  machine_t m(isa);
  const std::array<uint32_t, 4> input{0x00112233, 0x44556677, 0x8899aabb, 0xccddeeff};
  const std::array<uint32_t, 4> key{0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f};
  const std::array<uint32_t, 4> expected{0x00102030, 0x40506070, 0x8090a0b0, 0xc0d0e0f0};
  const auto opcode = p.aes | (9u << 20) | (8u << 7);

  if (!check(m.set_vl(p.set_vl, 4, 2u << 3), "set VL to four 32-bit elements"))
    return false;
  for (unsigned i = 0; i < input.size(); ++i) {
    m.proc.VU.elt<uint32_t>(8, i, true) = input[i];
    m.proc.VU.elt<uint32_t>(9, i, true) = key[i];
  }

  if (!check(m.step(opcode, p.aes_size), "execute AES AddRoundKey and advance by its encoded length"))
    return false;
  for (unsigned i = 0; i < expected.size(); ++i) {
    if (!check(m.proc.VU.elt<uint32_t>(8, i) == expected[i], "compare the AES result") ||
        !check(m.proc.VU.elt<uint32_t>(9, i) == key[i], "preserve the source key"))
      return false;
  }

  if (!check(m.set_vl(p.set_vl, 3, 2u << 3), "set VL to an incomplete AES element group") ||
      !check(m.illegal(opcode, p.aes_size), "trap on an incomplete AES element group"))
    return false;

  const std::string isa_without_crypto = base_isa + p.isa_without_crypto;
  machine_t disabled(isa_without_crypto);
  return check(disabled.set_vl(p.set_vl, 4, 2u << 3), "set VL for the disabled AES test") &&
         check(disabled.illegal(opcode, p.aes_size), "trap when the crypto extension is disabled");
}

}

int main(int argc, char** argv)
{
  const std::string_view selected = argc == 2 ? argv[1] : "default";
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [VK|WK|YK]\n", argv[0]);
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
      const bool ok = test_crypto(profile, base_isa);
      printf("%s RV%u: %s\n", profile.name, xlen, ok ? "passed" : "FAILED");
      passed &= ok;
    }
  }
  if (!matched)
    fprintf(stderr, "Unknown profile. Select VK, WK, or YK.\n");
  return matched && passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
