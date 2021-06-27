#include "ucentral.h"

static struct blob_buf rpc_buf;

enum {
	JSONRPC_VER,
	JSONRPC_METHOD,
	JSONRPC_PARAMS,
	__JSONRPC_MAX,
};

static int
jsonrpc_verify_serial(struct uc_client *client, struct blob_attr **rpc)
{
	enum {
		PARAMS_SERIAL,
		__PARAMS_MAX,
	};

	static const struct blobmsg_policy params_policy[__PARAMS_MAX] = {
		[PARAMS_SERIAL] = { .name = "serial", .type = BLOBMSG_TYPE_STRING },
	};

        struct blob_attr *tb[__PARAMS_MAX] = {};
	char *CN;

	blobmsg_parse(params_policy, __PARAMS_MAX, tb, blobmsg_data(rpc[JSONRPC_PARAMS]),
                      blobmsg_data_len(rpc[JSONRPC_PARAMS]));

	if (!tb[PARAMS_SERIAL]) {
                ULOG_ERR("%s: bad serial\n", client->CN);
		return -1;
	}

	CN = blobmsg_get_string(tb[PARAMS_SERIAL]);

	if (strcmp(CN, client->CN)) {
                ULOG_ERR("%s: CN does not match -> %s\n", client->CN, CN);
                return -1;
        }

	return 0;
}

void
jsonrpc_handle(struct uc_client *client, char *cmd)
{
	static const struct blobmsg_policy jsonrpc_policy[__JSONRPC_MAX] = {
		[JSONRPC_VER] = { .name = "jsonrpc", .type = BLOBMSG_TYPE_STRING },
		[JSONRPC_METHOD] = { .name = "method", .type = BLOBMSG_TYPE_STRING },
		[JSONRPC_PARAMS] = { .name = "params", .type = BLOBMSG_TYPE_TABLE },
	};

	struct blob_attr *rpc[__JSONRPC_MAX] = {};
	char *method;

	ULOG_INFO("RX: %s\n", cmd);
	blob_buf_init(&rpc_buf, 0);
	if (!blobmsg_add_json_from_string(&rpc_buf, cmd)) {
		ULOG_ERR("%s: failed to parse command\n", client->CN);
		return;
	}

	blobmsg_parse(jsonrpc_policy, __JSONRPC_MAX, rpc, blob_data(rpc_buf.head), blob_len(rpc_buf.head));
	if (!rpc[JSONRPC_VER] || !rpc[JSONRPC_METHOD] || !rpc[JSONRPC_PARAMS] ||
	     strcmp(blobmsg_get_string(rpc[JSONRPC_VER]), "2.0")) {
		ULOG_ERR("%s: received invalid jsonrpc call\n", client->CN);
		return;
	}

	if (jsonrpc_verify_serial(client, rpc))
		return;

	method = blobmsg_get_string(rpc[JSONRPC_METHOD]);

	if (!strcmp(method, "connect")) {
		client->connected = 1;
		ULOG_INFO("%s: client authenticated itself\n", client->CN);
	}


	if (!client->connected) {
		ULOG_ERR("%s: client is not authorized yet\n", client->CN);
		return;
	}

	ucode_run(client, cmd);
}
