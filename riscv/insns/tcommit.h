require_extension(EXT_XT);
require(p->transaction_active());
if (!p->commit_transaction())
  set_pc_and_serialize(p->abort_transaction(TX_CONFLICT));
