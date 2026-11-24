VI_VFP_NCVT_FP_TO_OFP8(
    {
      vd = P.VU.altfmt ? f32_to_e5m2(vs2, true) : f32_to_e4m3(vs2, true);
    },
    { require_extension(EXT_ZVFOFP8MIN); }
)
