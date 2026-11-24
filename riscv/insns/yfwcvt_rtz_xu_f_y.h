VI_NON_ALTFMT_INSN

VI_VFP_WCVT_FP_TO_INT(
  { vd = f16_to_ui32(vs2, softfloat_round_minMag, true); },
  { vd = f32_to_ui64(vs2, softfloat_round_minMag, true); },
  { require_extension(EXT_ZVFH); },
  { require(p->get_isa().get_zvf()); },
  uint
)
