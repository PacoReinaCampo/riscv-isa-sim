if (p->extension_enabled(EXT_ZVFBFA) && P.VU.vsew >= 16 && P.VU.altfmt)
  require(false);

VI_VFP_WCVT_INT_TO_FP(
  { vd = P.VU.altfmt ? f16(i32_to_bf16(vs2)) : i32_to_f16(vs2); },
  { vd = i32_to_f32(vs2); },
  { vd = i32_to_f64(vs2); },
  { require_zvfbfa_or_zvfh; },
  { require(p->get_isa().get_zvf()); },
  { require(p->get_isa().get_zvd()); },
  int
)
