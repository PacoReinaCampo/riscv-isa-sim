require_extension(EXT_XU);
require_novirt();
require(STATE.prv == PRV_U && STATE.user_trap_frame && STATE.user_trap_satp == STATE.satp->read());
STATE.ustatus->write(set_field(STATE.ustatus->read(), USTATUS_UIE, STATE.user_trap_saved_enable));
STATE.user_trap_frame = false;
set_pc_and_serialize(STATE.user_trap_resume);
