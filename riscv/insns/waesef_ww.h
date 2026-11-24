#include "zvkned_ext_macros.h"
#include "zvk_ext_macros.h"

require_vaes_vv_constraints;

VI_ZVK_VD_VS2_NOOPERANDS_PRELOOP_EGU32x4_NOVM_LOOP(
  {},
  {},
  {

    EGU8x16_t aes_state = P.VU.elt_group<EGU8x16_t>(vd_num, idx_eg);
    const EGU8x16_t round_key = P.VU.elt_group<EGU8x16_t>(vs2_num, idx_eg);

    VAES_SUB_BYTES(aes_state);

    VAES_SHIFT_ROWS(aes_state);

    EGU8x16_XOREQ(aes_state, round_key);

    EGU8x16_t &vd = P.VU.elt_group<EGU8x16_t>(vd_num, idx_eg, true);
    EGU8x16_COPY(vd, aes_state);
  }
);
