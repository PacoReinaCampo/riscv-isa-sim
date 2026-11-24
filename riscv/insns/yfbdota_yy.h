VI_VFP_BASE;
ZVBDOT_INIT(1);

switch (P.VU.vsew) {
  case 32: {

    require_extension(EXT_ZVFBDOTA32F);
    auto macc = [](auto a, auto b, auto c) { return f32_add(c, f32_mul(a, b)); };
    ZVBDOT_GENERIC_LOOP(float32_t, float32_t, float32_t, macc);
    break;
  }
  default: require(false);
}

VECTOR_END;
