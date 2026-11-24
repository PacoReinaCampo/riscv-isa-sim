if (p->extension_enabled(EXT_ZVFBFA) && P.VU.vsew >= 16 && P.VU.altfmt)
  require(false);

VI_VFP_NCVT_FP_TO_INT(
  { vd = VFP_16_TOINT(uint8_t, vs2, softfloat_round_minMag, true); },
  { vd = float_to_int<uint16_t>(vs2, softfloat_round_minMag, true); },
  { vd = f64_to_ui32(vs2, softfloat_round_minMag, true); },
  { require_zvfbfa_or_zvfh; },
  { require(p->get_isa().get_zvf()); },
  { require(p->get_isa().get_zvd()); },
  uint
)
