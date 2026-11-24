require_extension(EXT_XT);
require(p->transaction_active());
set_pc_and_serialize(p->abort_transaction((insn.csr() << 16) | TX_EXPLICIT));
