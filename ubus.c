#include <libubus.h>
#include <libubox/blobmsg_json.h>

#include "ucentral.h"

static struct ubus_auto_conn conn;
static struct blob_buf u;

enum {
	SERIAL,
	__SERIAL_MAX,
};

static const struct blobmsg_policy serial_policy[__SERIAL_MAX] = {
	[SERIAL] = { .name = "serial", .type = BLOBMSG_TYPE_STRING },
};

enum {
	REQUEST_SERIAL,
	REQUEST_TYPE,
	__REQUEST_MAX,
};

static const struct blobmsg_policy request_policy[__REQUEST_MAX] = {
	[REQUEST_SERIAL] = { .name = "serial", .type = BLOBMSG_TYPE_STRING },
	[REQUEST_TYPE] = { .name = "type", .type = BLOBMSG_TYPE_STRING },
};

static void ucode_ubus_cb(char *state, void *priv)
{
	blobmsg_add_json_from_string(&u, state);
}

static int ubus_ucode_cb(struct ubus_context *ctx,
			 struct ubus_object *obj,
			 struct ubus_request_data *req,
			 const char *method,
			 struct blob_attr *msg)
{
	char *json = NULL;

	if (msg)
		json = blobmsg_format_json(msg, true);

	blob_buf_init(&u, 0);
	if (ucode_ubus(U_UBUS, ucode_ubus_cb, method, json))
		return UBUS_STATUS_INVALID_ARGUMENT;

	if (blobmsg_len(u.head))
		ubus_send_reply(ctx, req, u.head);

	if (json)
		free(json);

	return UBUS_STATUS_OK;
}

static const struct ubus_method ucentral_methods[] = {
	UBUS_METHOD("state", ubus_ucode_cb, serial_policy),
	UBUS_METHOD("health", ubus_ucode_cb, serial_policy),
	UBUS_METHOD("blink", ubus_ucode_cb, serial_policy),
	UBUS_METHOD("reboot", ubus_ucode_cb, serial_policy),
	UBUS_METHOD("request", ubus_ucode_cb, request_policy),
	UBUS_METHOD_NOARG("connected", ubus_ucode_cb),
	UBUS_METHOD_NOARG("devices", ubus_ucode_cb),
};

static struct ubus_object_type ubus_object_type =
        UBUS_OBJECT_TYPE("ucentral", ucentral_methods);

struct ubus_object ubus_object = {
        .name = "ucentral",
        .type = &ubus_object_type,
        .methods = ucentral_methods,
        .n_methods = ARRAY_SIZE(ucentral_methods),
};

static void ubus_connect_handler(struct ubus_context *ctx)
{
        ubus_add_object(ctx, &ubus_object);
}

void ubus_init(void)
{
        conn.cb = ubus_connect_handler;
        ubus_auto_connect(&conn);
}

void ubus_deinit(void)
{
        ubus_auto_shutdown(&conn);
}

