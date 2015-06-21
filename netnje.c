/*
 * Copyright (c) 2015 Kenji Aoyama.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TARGET_HOST	"nje106.in.nk-home.net"

/* prototypes */
void usage(void);
int main(int, char *[]);

extern char *optarg;
extern int optind;

int vflag = 0;
char del[3] = {0x0d, 0x0a, 0x00};	/* delimiter, as a string */

int
main(int argc, char *argv[])
{
	int ch, i, sock;
	char buf[160], stamp[10];
	struct sockaddr_in sa;
	struct hostent *he;
	ssize_t sent;
	time_t now;

	while ((ch = getopt(argc, argv, "v")) != -1) {
		switch(ch) {
		case 'v':
			vflag = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(5000);

	he = gethostbyname(TARGET_HOST);
	if (he == NULL) {
		perror("gethostbyname");
		return -1;
	}
	memcpy(&sa.sin_addr, he->h_addr_list[0], sizeof(sa.sin_addr));

	/*
	 * Prepare sending data
	 *
	 * Message format:
	 *   {delimiter} {timestamp} {contents} {delimiter}
	 *     delimiter: 2 bytes
	 *       \r \n (CR LF)
	 *     timestamp: 8 bytes
	 *       MMDDhhmm in ASCII characters
	 *     contents: up to 128 bytes
	 *       ASCII/Shift-JIS string
	 *       it may contain some attributes and control commands
	 * See http://http://www.hydra.cx/msgbd_code.html
	 */
	time(&now);
	strftime(stamp, sizeof(stamp), "%m%d%H%M", localtime(&now));

	strlcpy(buf, del, sizeof(buf));
	strlcat(buf, stamp, sizeof(buf));

	for (i = 0; i < argc; i++)
		strlcat(buf, argv[i], sizeof(buf));
	if (strlen(buf) > 138) { /* delimiter + timestamp + 128 */
		fprintf(stderr, "too long message\n");
		exit(1);
	}

	strlcat(buf, del, sizeof(buf));

	if (vflag)
		printf("%s", buf);

	sent = sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&sa,
	    sizeof(struct sockaddr_in));

	if (sent == -1) {
		perror("sendto");
		close(sock);
		return -1;
	}

	close(sock);
	return 0;
}

void
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s [-v]\n", __progname);
	exit(1);
}
