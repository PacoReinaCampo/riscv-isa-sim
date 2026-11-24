VI_XI_LOOP_CARRY
({
  res = (((op_mask & vs2) - (op_mask & rs1) - carry) >> sew) & 0x1u;
})
