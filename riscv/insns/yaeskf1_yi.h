#include "zvk_ext_macros.h"
#include "zvkned_ext_macros.h"

require_vaeskf_vi_constraints;

static constexpr uint8_t kRoundConstants[10] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

VI_ZVK_VD_VS2_ZIMM5_EGU32x4_NOVM_LOOP(
  {},

  const reg_t zimm4 = zimm5 & 0xF;

  const reg_t round = ((1 <= zimm4) && (zimm4 <= 10)) ? zimm4 : (zimm4 ^ 0x8);
  const uint32_t rcon = kRoundConstants[round - 1];,

  {

    uint32_t temp = vs2[3];
    temp = (temp >> 8) | (temp << 24);
    temp = (((uint32_t)AES_ENC_SBOX[(temp >> 24) & 0xFF] << 24) |
            ((uint32_t)AES_ENC_SBOX[(temp >> 16) & 0xFF] << 16) |
            ((uint32_t)AES_ENC_SBOX[(temp >>  8) & 0xFF] <<  8) |
            ((uint32_t)AES_ENC_SBOX[(temp >>  0) & 0xFF] <<  0));
    temp = temp ^ rcon;

    const uint32_t w0 = vs2[0] ^ temp;
    const uint32_t w1 = vs2[1] ^ w0;
    const uint32_t w2 = vs2[2] ^ w1;
    const uint32_t w3 = vs2[3] ^ w2;

    SET_EGU32x4_LE(vd, w0, w1, w2, w3);
  }
);
