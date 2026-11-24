VI_XI_LOOP_CARRY
({
  res = (((op_mask & rs1) + (op_mask & vs2) + carry) >> sew) & 0x1u;
})
