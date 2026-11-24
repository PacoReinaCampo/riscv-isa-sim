#include "zvk_ext_macros.h"

require_zvkb;

const reg_t mask = P.VU.vsew - 1;

const reg_t rshift = insn.v_zimm6() & mask;
const reg_t lshift = (-rshift) & mask;

VI_V_ULOOP
({
  vd = (vs2 << lshift) | (vs2 >> rshift);
})
