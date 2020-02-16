#define ARTNET_DEFAULT_PORT "6454"
#define ARTNET_OPCODE_DMX 0x0050
#define ARTNET_CHANNELS 3
#define DISPLAY_DEFAULT_PORT "9000"

#pragma pack(push, 1)
typedef struct /*_tx_header*/ {
	uint8_t sync;
	uint8_t lines;
	uint8_t destination;
	uint8_t eoh;
	uint8_t flags;
} tx_header;

typedef struct /*_tx_page_zero*/ {
	uint8_t index[3];
	uint8_t hours[2];
	uint8_t minutes[2];
	uint8_t seconds[2];
	uint8_t dom[2];
	uint8_t month[2];
	uint8_t dow[2];
} tx_pagezero_hdr;

typedef struct /*_tx_page*/ {
	uint8_t index[3];
	uint8_t tempo;
	uint8_t function;
	uint8_t flags;
} tx_page_hdr;

typedef struct /*_message*/ {
	size_t pages;
	char** text;
} message;

typedef struct /*_artnet_pkt*/ {
	uint8_t magic[8];
	uint16_t opcode;
	uint16_t version;
	uint8_t sequence;
	uint8_t port;
	uint8_t universe;
	uint8_t net;
	uint16_t length;
	uint8_t data[512];
} artnet_pkt;
#pragma pack(pop)

//MSG tx_header pages EOT(0x04) CSUM

//default header flags: reserved bits
#define HFLAGS_DEFAULT 0xC0
#define HFLAG_INTERRUPT 0x02
#define HFLAG_CONTINUE 0x04

//default tempo flags: show always
#define PFLAGS_TEMPO_DEFAULT (HFLAGS_DEFAULT | 0x20)
#define PFLAGS_FUNCTION_DEFAULT (HFLAGS_DEFAULT)
//default page flags: extended charset
#define PFLAGS_DEFAULT (0xA0)

#define PFLAG_COMBINE_12 0x01
#define PFLAG_COMBINE_34 0x02
#define PFLAG_COMBINE_56 0x04
#define PFLAG_COMBINE_78 0x08
#define PFLAG_CENTER 0x10

unsigned page_interval[] = {2, 5, 10, 20, 30, 45, 60, 90, 120};
