require_rv64;
require_extension(EXT_ZBP);
WRITE_RD(sext_xlen(xperm(RS1, RS2, 5, xlen)));
