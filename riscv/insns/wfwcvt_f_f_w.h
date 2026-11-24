VI_VFP_WCVT_FP_TO_FP(
  { vd = any_f16_to_f32(vs2, P.VU.altfmt); },
  { vd = f32_to_f64(vs2); },
  { require_zvfbfa_or_zvfhmin },
  { require(p->get_isa().get_zvd()); }
)
