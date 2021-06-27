/*
 * lws-minimal-ws-server
 *
 * Written in 2010-2019 by Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * This demonstrates the most minimal http server you can make with lws,
 * with an added websocket chat server.
 *
 * To keep it simple, it serves stuff in the subdirectory "./mount-origin" of
 * the directory it was started in.
 * You can change that by changing mount.origin.
 */

#include <libwebsockets.h>
#include <libubox/uloop.h>
#include <string.h>
#include <signal.h>

#define LWS_PLUGIN_STATIC
#include "websocket.c"

static struct lws_protocols protocol_websocket[] = {
	LWS_PLUGIN_PROTOCOL_MINIMAL,
	{ NULL, NULL, 0, 0 }
};

int main(int argc, const char **argv)
{
	struct lws_context *context;
	struct lws_context_creation_info info;
	const char *p;
	int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = 7;

	if (ucode_init())
		exit(1);

	lws_set_log_level(logs, NULL);

	memset(&info, 0, sizeof info);
	info.iface = "eno1";
	info.port = 15002;
	info.protocols = protocol_websocket;
	info.vhost_name = "uCentral";
	info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
	info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	info.options |= LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT;
	info.options |= LWS_SERVER_OPTION_ULOOP;
	info.ssl_cert_filepath = "localhost-100y.cert";
	info.ssl_private_key_filepath = "localhost-100y.key";

	uloop_init();
	ubus_init();
	context = lws_create_context(&info);
	uloop_run();
	ubus_deinit();
	lws_context_destroy(context);

	return 0;
}
