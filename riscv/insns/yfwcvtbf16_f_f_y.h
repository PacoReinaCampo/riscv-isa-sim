VI_VFP_WCVT_OFP8_BF16_FP(
  { vd = f16(P.VU.altfmt ? e5m2_to_bf16(vs2) : e4m3_to_bf16(vs2)); },
  { vd = bf16_to_f32(bf16(vs2)); },
  { require(p->extension_enabled(EXT_ZVFOFP8MIN)); },
  { require_extension(EXT_ZVFBFMIN); }
)
