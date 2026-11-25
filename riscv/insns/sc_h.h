require_extension(EXT_ZABHLRSC);
WRITE_RD(!MMU.store_conditional<uint16_t>(RS1, RS2));
