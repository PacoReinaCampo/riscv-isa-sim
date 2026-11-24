#include "zvk_ext_macros.h"

require_zvkb;

const reg_t mask = P.VU.vsew - 1;

const reg_t rshift = ((reg_t)RS1) & mask;
const reg_t lshift = (-rshift) & mask;

VI_V_ULOOP
({
  vd = (vs2 << lshift) | (vs2 >> rshift);
})
