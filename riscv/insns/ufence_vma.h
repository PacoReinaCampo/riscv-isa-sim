require_extension(EXT_XU);
require_novirt();
require(STATE.prv == PRV_U);
const reg_t asid_mask = xlen == 32 ? SATP32_ASID : SATP64_ASID;
const reg_t asid = get_field(STATE.satp->read(), asid_mask);
require(insn.rs2() == 0 || (RS2 & get_field(asid_mask, asid_mask)) == asid);
if (insn.rs1() == 0)
  MMU.flush_tlb();
else
  MMU.flush_tlb_address(RS1);
