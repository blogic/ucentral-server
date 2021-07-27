#ifndef _UCENTRAL_H__
#define _UCENTRAL_H__

#include <string.h>
#include <ctype.h>

#include <libwebsockets.h>

#include <libubox/ulog.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <libubox/avl-cmp.h>
#include <libubox/avl.h>

struct uc_client {
	struct avl_node avl;
	struct lws *wsi;

	bool connected;
	char ip[64];
	char CN[64];
};

extern void jsonrpc_handle(struct uc_client *client, char *cmd);

enum {
	U_INIT = 0,
	U_DISPATCH,
	U_DISCONNECT,
	U_CONNECTED,
	U_UBUS,
	__U_MAX,
};

extern int ucode_init(void);
extern int ucode_rpc(int function, void (*cb)(char *, void *),
		     struct uc_client *client, char *in);

extern int ucode_ubus(int function, void (*cb)(char *, void*),
		      const char *method, char *in);

extern void ubus_init(void);
extern void ubus_deinit(void);

void ws_client_send(const char *serial, char *msg, size_t len);

#endif
