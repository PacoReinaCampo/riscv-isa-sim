VI_VFP_CVT_FP_TO_INT(
  { vd = float_to_int<int16_t>(vs2, softfloat_roundingMode, true); },
  { vd = f32_to_i32(vs2, softfloat_roundingMode, true); },
  { vd = f64_to_i64(vs2, softfloat_roundingMode, true); },
  int
)
