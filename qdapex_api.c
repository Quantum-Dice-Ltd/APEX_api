/* qdapex_api.c - communication with APEX randomness generator over IPv6 UDP */

#include "qdapex_api.h"

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <limits.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static unsigned dbg_idx;

static int sendrq(QDAPEX *qu, unsigned rqlen)
{
 const unsigned MAXREQ = sizeof(qu->gotcur) * CHAR_BIT;
 if (rqlen > MAXREQ) rqlen = MAXREQ;	// limit so qu->gotcur bitmask can hold enough bits
 char rq[] = { 0xce, 0x5d, 0x89, 0x73, 'Q', 'R', 0, 0, 0, 0, };
 rq[6] = qu->nextid >> 8;
 rq[7] = qu->nextid;
 rq[8] = rqlen >> 8;
 rq[9] = rqlen;
 ssize_t nw;
 if (!qu->connected) {
	struct sockaddr_in6 sin6;
	sin6.sin6_family = AF_INET6;
	sin6.sin6_flowinfo = 0;
	memset(&sin6.sin6_addr, 0, sizeof(sin6.sin6_addr));
	sin6.sin6_scope_id = dbg_idx;
	sin6.sin6_addr.s6_addr[0] = 0xff;
	sin6.sin6_addr.s6_addr[1] = 0x02;
	sin6.sin6_addr.s6_addr[15] = 0x01;
	sin6.sin6_port = htons(20804);
	nw = sendto(qu->sock, rq, sizeof(rq), 0, (struct sockaddr *) &sin6, sizeof(sin6));
 } else {
	nw = send(qu->sock, rq, sizeof(rq), 0);
 }
 if (nw == -1) {
	perror("send() failed");
	fprintf(stderr, "Cannot send request to APEX UDP\n");
	return 1;
 }
 if (qu->dbgfil) fprintf(qu->dbgfil, "\n%04x:%u>", qu->nextid, rqlen);
 ++qu->nextid;
 qu->gotprev = qu->gotcur;
 if (rqlen == MAXREQ) qu->gotcur = 0;	// NB: C does not allow shift as big as the whole word
 else qu->gotcur = ~0u << rqlen;	// reject any blocks not asked for
 qu->expected += rqlen;
 return 0;
}

ssize_t qdapex_read(QDAPEX *qu, char *buf, ssize_t len)
{
 char *bp = buf, *ep = buf + len;
 int needwait = 0;	// we're inveterate optimists - hope that there's pending data
 while (bp < ep) {
	if (qu->expected < 8) {
			// send a request
		unsigned rqlen = ep - bp;
		unsigned odd = rqlen & 1023;
		rqlen >>= 10;
		if (odd) ++rqlen;
		rqlen += 8;	// to keep 8 blocks pending
		rqlen -= qu->expected;
		if (sendrq(qu, rqlen)) return -1;
	}
	if (needwait) {	// we could actually do this always, but it's a small optimisation
		struct pollfd pfd;
		pfd.fd = qu->sock;
		pfd.events = POLLIN;
		if (qu->dbgfil) putc('P', qu->dbgfil);
		int pr = poll(&pfd, 1, 100);
		if (pr == -1) {
			perror("poll() failed");
			fprintf(stderr, "Cannot wait for APEX UDP socket to be ready\n");
			return -1;
		}
		if (!pr) {
			if (qu->dbgfil) fprintf(qu->dbgfil, "Timeout (expected %u)\n", qu->expected);
			qu->expected = 0;	// must have lost them all
			break;	// send a request for the outstanding bytes (which may be a re-transmission)
		}
		needwait = 0;	// now keep reading until there's nothing
	}
	struct sockaddr_storage sa;
	socklen_t salen = sizeof(sa);
	unsigned char rxbuf[2048];
	ssize_t nr;
	if (!qu->connected) {
		nr = recvfrom(qu->sock, rxbuf, sizeof(rxbuf), MSG_DONTWAIT, (struct sockaddr *) &sa, &salen);
	} else {
		nr = recv(qu->sock, rxbuf, sizeof(rxbuf), MSG_DONTWAIT);
	}
	if (nr == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			needwait = 1;
			if (qu->dbgfil) putc('T', qu->dbgfil);
			continue;
		}
		perror("recv() failed");
		fprintf(stderr, "Cannot receive from QRNG APEX UDP\n");
		return -1;
	}
	if (rxbuf[0] == 'Q' && rxbuf[1] == 'r') {
		unsigned id = (rxbuf[2] << 8) + rxbuf[3];
		unsigned more = (rxbuf[4] << 8) + rxbuf[5];	// also acts as block ID within a request
		unsigned did = (qu->nextid - id) & 0xffff;
		const unsigned MAXREQ = sizeof(qu->gotcur) * CHAR_BIT;
		unsigned *pos;
		if (did == 1) {
			if (qu->dbgfil) putc('r', qu->dbgfil);
			pos = &qu->gotcur;
		} else if (did == 2) {
			if (qu->dbgfil) putc('R', qu->dbgfil);
			pos = &qu->gotprev;
		} else {
			if (qu->dbgfil) fprintf(qu->dbgfil, "<%04xm%04x!\n", id, more);
			continue;
		}
		if (more >= MAXREQ) {
			if (qu->dbgfil) fprintf(qu->dbgfil, "sub-block more is %u >= max\n", more);
			continue;
		}
		unsigned mask = 1 << more;
		if (*pos & mask) {
			if (qu->dbgfil) fprintf(qu->dbgfil, "sub-block nx - %u dup %u\n", did, more);
			continue;
		}
		*pos |= mask;
		if (!qu->connected) {
				// we now know the server's address, so restrict the socket
				// to only accept from that address and to not need to
				// specify the sending address on each send()
			if (connect(qu->sock, (struct sockaddr *) &sa, salen)) {
				perror("connect() failed");
				fprintf(stderr, "Cannot connect socket for QDRNG APEX UDP\n");
				return -1;
			}
			qu->connected = 1;
		}
		if (qu->dbgfil) putc('r', qu->dbgfil);
		nr -= 6;
		if (nr > ep - bp) nr = ep - bp;
		memcpy(bp, rxbuf + 6, nr);
		bp += nr;
		if (!qu->expected) {
			if (qu->dbgfil) fprintf(qu->dbgfil, "unexpected\n");
			break;
		}
		--qu->expected;
		if (qu->dbgfil && !qu->expected) putc('+', qu->dbgfil);
	} else if (qu->dbgfil) fprintf(qu->dbgfil, "rx ? %02x %02x\n", rxbuf[0], rxbuf[1]);
 }
 return bp - buf;
}

static int stdsock(const char *eth)
{
 unsigned idx = if_nametoindex(eth);
 if (!idx) {
	perror("if_nametoindex() failed");
	fprintf(stderr, "Cannot find interface %s\n", eth);
	return -1;
 }
 dbg_idx = idx;
 int fd = socket(AF_INET6, SOCK_DGRAM, 0);
 if (fd == -1) {
	perror("socket() failed");
	fprintf(stderr, "Cannot create socket for QDRNG APEX UDP\n");
	return -1;
 }
 struct sockaddr_in6 sin6;
 sin6.sin6_family = AF_INET6;
 sin6.sin6_flowinfo = 0;
 sin6.sin6_port = 0;
 memset(&sin6.sin6_addr, 0, sizeof(sin6.sin6_addr));
 sin6.sin6_scope_id = idx;
 if (bind(fd, (struct sockaddr *) &sin6, sizeof(sin6)) == -1) {
	perror("bind() failed");
	fprintf(stderr, "Cannot prepare socket for QDRNG APEX UDP\n");
	close(fd);
	return -1;
 }
 return fd;
}

int qdapex(QDAPEX *qu, const char *eth)
{
 ssize_t nr = getrandom(&qu->nextid, sizeof(qu->nextid), 0);
 if (nr == -1) {
	perror("getrandom() failed");
	fprintf(stderr, "qdapex() cannot pick random initial ID\n");
	return 1;
 }
 qu->sock = stdsock(eth);
 qu->connected = 0;
 qu->expected = 0;
 qu->gotcur = 0;
 qu->gotprev = 0;
 qu->dbgfil = 0;
 return qu->sock == -1;
}

FILE *qdapex_debug(QDAPEX *qu, FILE *dbgfil)
{
 FILE *f = qu->dbgfil;
 qu->dbgfil = dbgfil;
 return f;
}

void qdapex_dtor(QDAPEX *qu)
{
 close(qu->sock);
}
