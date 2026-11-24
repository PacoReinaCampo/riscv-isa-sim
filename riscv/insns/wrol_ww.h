#include "zvk_ext_macros.h"

require_zvkb;

const reg_t mask = P.VU.vsew - 1;

VI_VV_ULOOP
({

  const reg_t lshift = vs1 & mask;
  const reg_t rshift = (-lshift) & mask;
  vd = (vs2 << lshift) | (vs2 >> rshift);
})
