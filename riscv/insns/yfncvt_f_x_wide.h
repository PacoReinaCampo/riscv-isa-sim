VI_NON_ALTFMT_INSN

VI_VFP_NCVT_INT_TO_FP(
  { vd = i32_to_f16(vs2); },
  { vd = i64_to_f32(vs2); },
  { require_extension(EXT_ZVFH); },
  { require(p->get_isa().get_zvf()); },
  int
)
