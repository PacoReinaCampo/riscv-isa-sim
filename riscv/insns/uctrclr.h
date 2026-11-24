require_extension(EXT_XUSCTR);
require_novirt();
require(STATE.prv == PRV_U);
STATE.user_ctr.erase(STATE.satp->read());
