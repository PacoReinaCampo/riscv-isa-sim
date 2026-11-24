VI_VFP_VF_LOOP
({
  vd = VFP_FP_OP_16(sub, rs1, vs2);
},
{
  vd = f32_sub(rs1, vs2);
},
{
  vd = f64_sub(rs1, vs2);
})
