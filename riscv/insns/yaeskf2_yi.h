#include "zvk_ext_macros.h"
#include "zvkned_ext_macros.h"

require_vaeskf_vi_constraints;

static constexpr uint8_t kRoundConstants[7] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,
};

VI_ZVK_VD_VS2_ZIMM5_EGU32x4_NOVM_LOOP(
  {},

  const reg_t zimm4 = zimm5 & 0xF;

  const reg_t round = ((2 <= zimm4) && (zimm4 <= 14)) ? zimm4 : (zimm4 ^ 0x8);,

  {

    uint32_t temp = vs2[3];

    const bool is_even_round = (round & 0x1) == 0;
    if (is_even_round) {
      temp = (temp >> 8) | (temp << 24);
    }
    temp = (((uint32_t)AES_ENC_SBOX[(temp >> 24) & 0xFF] << 24) |
            ((uint32_t)AES_ENC_SBOX[(temp >> 16) & 0xFF] << 16) |
            ((uint32_t)AES_ENC_SBOX[(temp >>  8) & 0xFF] <<  8) |
            ((uint32_t)AES_ENC_SBOX[(temp >>  0) & 0xFF] <<  0));

    if (is_even_round) {
      const uint32_t rcon = kRoundConstants[(round >> 1) - 1];
      temp = temp ^ rcon;
    }

    const uint32_t w0 = vd[0] ^ temp;
    const uint32_t w1 = vd[1] ^ w0;
    const uint32_t w2 = vd[2] ^ w1;
    const uint32_t w3 = vd[3] ^ w2;

    SET_EGU32x4_LE(vd, w0, w1, w2, w3);
  }
);
