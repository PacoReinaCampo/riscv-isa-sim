require_vector_novtype(false);
WRITE_RD(P.VU.set_vl(insn.rd(), insn.rs1(), RS1, RS2));
P.VU.vstart->write_raw(0);
