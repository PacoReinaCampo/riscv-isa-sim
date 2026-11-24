VI_VV_LOOP_CARRY
({
  res = (((op_mask & vs2) - (op_mask & vs1) - carry) >> sew) & 0x1u;
})
