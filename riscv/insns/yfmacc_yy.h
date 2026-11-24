require_zvfbfa

VI_VFP_VV_LOOP
({
  vd = VFP_MULADD_16(vs1, vs2, vd);
},
{
  vd = f32_mulAdd(vs1, vs2, vd);
},
{
  vd = f64_mulAdd(vs1, vs2, vd);
})
