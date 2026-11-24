require_extension(EXT_XXSCTR);
require_privilege(PRV_M);
STATE.machine_ctr.enabled = RS1 & 1;
