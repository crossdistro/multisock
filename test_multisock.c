#include <multisock.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int
main(int argc, char **argv)
{
	multisock_t sock;
	int sockfd;

	sock = multisock_create(MULTISOCK_ATTR_DEFAULT);
	if (sock == MULTISOCK_CREATE_FAILED) {
		perror("multisock_socket");
		exit(EXIT_FAILURE);
	}

	sockfd = multisock_connect_name(sock, "github.com", "http");

	{
		const char request[] = "GET / HTTP/1.0\r\nHost: github.com\r\n\r\n";
		const char *start = request;
		const char *end = request + strlen(request);
		size_t size;

		while ((size = send(sockfd, start, end - start, 0)) > 0)
			start += size;
		if (size == -1) {
			perror("write");
			exit(EXIT_SUCCESS);
		}
	}

	while (1) {
		char buffer[256] = { 0 };
		int size;

		size = recv(sockfd, buffer, sizeof buffer - 1, 0);
		switch (size) {
		case -1:
			perror("read");
			exit(EXIT_SUCCESS);
		case 0:
			exit(EXIT_FAILURE);
		default:
			printf("%d: <<<%s>>>\n", size, buffer);
		}
	}
}
