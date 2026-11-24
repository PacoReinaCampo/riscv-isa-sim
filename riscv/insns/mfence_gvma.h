require_extension(EXT_XX);
require_privilege(PRV_M);
require_extension('H');
(void)RS1;
(void)RS2;
MMU.flush_tlb();
