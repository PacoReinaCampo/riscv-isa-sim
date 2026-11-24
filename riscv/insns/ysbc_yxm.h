VI_XI_LOOP_WITH_CARRY
({
  vd = (uint128_t)((op_mask & vs2) - (op_mask & rs1) - carry);
})
