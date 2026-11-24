VI_VFP_CVT_FP_TO_INT(
  { vd = float_to_int<uint16_t>(vs2, softfloat_roundingMode, true); },
  { vd = f32_to_ui32(vs2, softfloat_roundingMode, true); },
  { vd = f64_to_ui64(vs2, softfloat_roundingMode, true); },
  uint
)
