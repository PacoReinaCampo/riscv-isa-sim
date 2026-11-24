require_extension(EXT_XT);
require(insn.rd() != 0);
CHECK_REG(insn.rd());
if (p->transaction_active()) {
  set_pc_and_serialize(p->abort_transaction(TX_NESTED));
} else {
  WRITE_RD(p->begin_transaction(insn.rd(), npc));
  set_pc_and_serialize(npc);
}
