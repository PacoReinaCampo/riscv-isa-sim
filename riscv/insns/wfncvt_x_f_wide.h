if (p->extension_enabled(EXT_ZVFBFA) && P.VU.vsew >= 16 && P.VU.altfmt)
  require(false);

VI_VFP_NCVT_FP_TO_INT(
  { vd = VFP_16_TOINT(int8_t, vs2, softfloat_roundingMode, true); },
  { vd = float_to_int<int16_t>(vs2, softfloat_roundingMode, true); },
  { vd = f64_to_i32(vs2, softfloat_roundingMode, true); },
  { require_zvfbfa_or_zvfh; },
  { require(p->get_isa().get_zvf()); },
  { require(p->get_isa().get_zvd()); },
  int
)
