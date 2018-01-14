#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <netdb.h>
#include <errno.h>

#include "multisock.h"

typedef struct _multisock_source *multisock_source_t;

struct _multisock_attr {
	int socktype;
	int protocol;
};

struct _multisock_source {
	struct _multisock_source *previous, *next;
	int sock;
};

struct _multisock {
	struct _multisock_attr attr;
	struct _multisock_source sources;
};

#define _foreach_source(_sock, _source) \
	for ( \
	    multisock_source_t _source = _sock->sources.next; \
		_source != &_sock->sources; \
		_source = _source->next)

static struct _multisock_attr default_attr = { .socktype = SOCK_STREAM, .protocol = IPPROTO_TCP };

multisock_t
multisock_create(multisock_attr_t attr)
{
	multisock_t sock = malloc(sizeof *sock);

	if (!sock)
		return MULTISOCK_CREATE_FAILED;

	memcpy(&sock->attr, attr ?: &default_attr, sizeof sock->attr);
	sock->sources.next = sock->sources.previous = &sock->sources;

	return sock;
}

multisock_source_t
_socket(multisock_t sock, int family)
{
	multisock_source_t source;

	source = calloc(1, sizeof *source);
	if (!source)
		goto fail;

	source->sock = socket(family, sock->attr.socktype, sock->attr.protocol);
	if (source->sock == -1)
		goto fail_socket;

	source->previous = sock->sources.previous;
	source->next = &sock->sources;
	source->previous->next = source;
	source->next->previous = source;

	return source;
fail_socket:
	free(source);
fail:
	return NULL;
}

int
multisock_listen(multisock_t sock, const struct sockaddr *sa, socklen_t salen, int backlog)
{
	multisock_source_t source;
	int status;

	source = _socket(sock, sa->sa_family);
	if (!source)
		goto fail;

	status = bind(source->sock, sa, salen);
	if (status == -1)
		goto fail_bind_listen;

	status = listen(source->sock, backlog);
	if (status == -1)
		goto fail_bind_listen;

	return source->sock;
fail_bind_listen:
	// TODO: We should probably remove the source if it can't be used.
fail:
	return -1;
}

int
multisock_accept(multisock_t sock, struct sockaddr *sa, socklen_t *salen)
{
	fd_set readfds;
	int nfds = -1;
	int status;

	FD_ZERO(&readfds);
	_foreach_source(sock, source) {
		FD_SET(source->sock, &readfds);
		if (source->sock > nfds)
			nfds = source->sock;
	}

	status = select(nfds, &readfds, NULL, NULL, NULL);
	if (status == -1)
		return -1;

	_foreach_source(sock, source)
		if (FD_ISSET(source->sock, &readfds))
			return accept(source->sock, sa, salen);

	return 0;
}

int
_source_pickup(multisock_source_t source)
{
	int sockfd = source->sock;

	source->next->previous = source->previous;
	source->previous->next = source->next;

	free(source);

	return sockfd;
}

int
multisock_connect(multisock_t sock, struct sockaddr *sa, socklen_t salen)
{
	multisock_source_t source;
	int status;

	source = _socket(sock, sa->sa_family);
	if (!source)
		goto fail_socket;

	status = connect(source->sock, sa, salen);
	if (status == -1)
		goto fail_connect;

	return _source_pickup(source);
fail_connect:
	close(_source_pickup(source));
fail_socket:
	return -1;
}

int
multisock_connect_name(multisock_t sock, const char *nodename, const char *servname)
{
	int sockfd;
	int status;
	struct addrinfo hints = {
		.ai_socktype = sock->attr.socktype,
		.ai_protocol = sock->attr.protocol
	};
	struct addrinfo *result;
	
	status = getaddrinfo(nodename, servname, &hints, &result);
	if (status)
		return -1;

	for (struct addrinfo *ai = result; ai; ai = ai->ai_next) {
		sockfd = multisock_connect(sock, ai->ai_addr, ai->ai_addrlen);
		if (sockfd != -1)
			break;
	}

	freeaddrinfo(result);
	return sockfd;
}

static void
_close_source(multisock_source_t source)
{
	// TODO: Don't leak file descriptors when close fails.
	close(source->sock);
	free(source);
}

void
multisock_close(multisock_t sock)
{
	multisock_source_t last = NULL;

	_foreach_source(sock, source) {
		if (last)
			_close_source(last);
		last = source;
	}
	_close_source(last);

	free(sock);
}

multisock_attr_t *
multisock_attr_create(void)
{
	return calloc(1, sizeof (multisock_attr_t *));
}

void
multisock_attr_set_socktype(multisock_attr_t attr, int socktype)
{
	attr->socktype = socktype;
}

void
multisock_attr_set_protocol(multisock_attr_t attr, int protocol)
{
	attr->protocol = protocol;
}

void
multisock_attr_free(multisock_attr_t attr)
{
	free(attr);
}
