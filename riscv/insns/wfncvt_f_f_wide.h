VI_VFP_NCVT_FP_TO_FP(
  { vd = P.VU.altfmt ? f16(f32_to_bf16(vs2)) : f32_to_f16(vs2); },
  { vd = f64_to_f32(vs2); },
  { require_zvfbfa_or_zvfhmin },
  { require(p->get_isa().get_zvd()); }
)
