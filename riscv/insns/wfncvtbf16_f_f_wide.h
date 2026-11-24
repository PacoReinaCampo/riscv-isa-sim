VI_VFP_NCVT_FP_BF16_OFP8(
  {
    vd = P.VU.altfmt ? bf16_to_e5m2(bf16(vs2), false) : bf16_to_e4m3(bf16(vs2), false);
  },
  { vd = f16(f32_to_bf16(vs2)); },
  { require(p->extension_enabled(EXT_ZVFOFP8MIN)); },
  { require_extension(EXT_ZVFBFMIN); }
)
