/* License: https://creativecommons.org/share-your-work/public-domain/cc0/ */

#include <stdlib.h>
#include <netinet/ip.h>

typedef struct _multisock *multisock_t;
typedef struct _multisock_attr *multisock_attr_t;

#define MULTISOCK_ATTR_DEFAULT NULL
#define MULTISOCK_CREATE_FAILED NULL

multisock_t multisock_create(const multisock_attr_t attr);
int multisock_listen(multisock_t sock, const struct sockaddr *sa, socklen_t salen, int backlog);
int multisock_accept(multisock_t sock, struct sockaddr *sa, socklen_t *salen);
int multisock_connect(multisock_t sock, struct sockaddr *sa, socklen_t salen);
int multisock_connect_name(multisock_t sock, const char *nodename, const char *servname);

multisock_attr_t *multisock_attr_create(void);
void multisock_attr_set_socktype(multisock_attr_t attr, int socktype);
void multisock_attr_set_protocol(multisock_attr_t attr, int protocol);
// TODO: We probably want to treat nonblock separately from socktype.
void multisock_attr_free(multisock_attr_t attr);
