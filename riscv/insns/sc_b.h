require_extension(EXT_ZABHLRSC);
WRITE_RD(!MMU.store_conditional<uint8_t>(RS1, RS2));
