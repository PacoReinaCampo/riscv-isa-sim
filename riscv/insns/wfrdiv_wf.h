VI_NON_ALTFMT_INSN

VI_VFP_VF_LOOP
({
  vd = f16_div(rs1, vs2);
},
{
  vd = f32_div(rs1, vs2);
},
{
  vd = f64_div(rs1, vs2);
})
