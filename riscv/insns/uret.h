require_extension(EXT_XN);
require_novirt();
const reg_t status = STATE.ustatus->read();
STATE.ustatus->write(set_field(status | USTATUS_UPIE, USTATUS_UIE, bool(status & USTATUS_UPIE)));
STATE.user_trap_frame = false;
set_pc_and_serialize(STATE.uepc->read());
p->set_privilege(PRV_U, false);
