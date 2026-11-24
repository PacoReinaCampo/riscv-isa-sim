VI_VV_LOOP_NARROW
({
  VRM xrm = P.VU.get_vround_mode();
  uint64_t uint_max = UINT64_MAX >> (64 - P.VU.vsew);
  uint64_t sign_mask = UINT64_MAX << P.VU.vsew;
  uint128_t result = vs2_u;
  unsigned shift = vs1 & ((sew * 2) - 1);

  INT_ROUNDING(result, xrm, shift);

  result = result >> shift;

  if (result & sign_mask) {
    result = uint_max;
    P_SET_OV(1);
  }

  vd = result;
})
