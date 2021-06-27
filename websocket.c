#include "ucentral.h"

struct ws_msg {
	void *payload;
	size_t len;
};

struct ws_vhost {
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;

	struct uc_client *client_list;

//	struct ws_msg aws_msg;
	int current;
};

static void
ws_msg_free(void *_ws_msg)
{
	struct ws_msg *ws_msg = _ws_msg;

	free(ws_msg->payload);
	ws_msg->payload = NULL;
	ws_msg->len = 0;
}

static void
ws_send(char *_msg, void *priv )
{
	struct uc_client *client = (struct uc_client *) priv;
	int len = strlen(_msg) + 1;
	char *msg = malloc(len + LWS_PRE);

	strcpy(&msg[LWS_PRE], _msg);
	lws_write(client->wsi, &msg[LWS_PRE], len, LWS_WRITE_TEXT);
	free(msg);
}

static int
ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct uc_client *client =
			(struct uc_client *)user;
	struct ws_vhost *vhost =
			(struct ws_vhost *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi),
					lws_get_protocol(wsi));
	int m;
	uint8_t buf[1280];
	union lws_tls_cert_info_results *ci =
		(union lws_tls_cert_info_results *)buf;
	char *cn;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		vhost = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi),
				sizeof(struct ws_vhost));
		vhost->context = lws_get_context(wsi);
		vhost->protocol = lws_get_protocol(wsi);
		vhost->vhost = lws_get_vhost(wsi);
		break;
	case LWS_CALLBACK_ESTABLISHED:
		lws_get_peer_simple(wsi, client->ip, sizeof(client->ip));
		if (lws_tls_peer_cert_info(wsi, LWS_TLS_CERT_INFO_COMMON_NAME,
					   ci, sizeof(ci->ns.name))) {
			ULOG_ERR("failed to get common name, closing connection\n");
			return -1;
		}
		cn = strncpy(client->CN, ci->ns.name, sizeof(client->CN));
		while (*cn) {
			*cn = tolower(*cn);
			cn++;
		}
		ULOG_INFO("New Connection: %s / %s\n", client->ip, client->CN);
		lws_ll_fwd_insert(client, client_list, vhost->client_list);
		client->wsi = wsi;
		client->last = vhost->current;
		break;

	case LWS_CALLBACK_CLOSED:
		ULOG_INFO("close connection\n");
		lws_ll_fwd_remove(struct uc_client, client_list,
				  client, vhost->client_list);
		ucode_rpc(U_DISCONNECT, NULL, client, NULL);
		break;

	case LWS_CALLBACK_RECEIVE:
		((char *)in)[len] = '\0';
		ucode_rpc(U_DISPATCH, ws_send, client, in);
		break;
	}

	return 0;
}

#define LWS_PLUGIN_PROTOCOL_MINIMAL \
	{ \
		"ucentral-broker", \
		ws_callback, \
		sizeof(struct uc_client), \
		32 * 1024, \
		0, NULL, 0 \
	}
