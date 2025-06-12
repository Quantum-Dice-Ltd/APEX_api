#ifndef INCLUDED_QDAPEX
#define INCLUDED_QDAPEX

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct {
	int sock;
	int connected;
	FILE *dbgfil;
	unsigned expected;	// blocks already requested but not consumed
	unsigned gotprev;	// bitmask of blocks received from previous request
	unsigned gotcur;	// bitmask of blocks received from current request
	uint16_t nextid;	// id of next request
} QDAPEX;

ssize_t qdapex_read(QDAPEX *qu, char *buf, ssize_t len);

int qdapex(QDAPEX *qu, const char *eth);

FILE *qdapex_debug(QDAPEX *qu, FILE *dbgfil);

void qdapex_dtor(QDAPEX *qu);

#endif
