#include "ucentral.h"

#include <ucode/compiler.h>
#include <ucode/lib.h>
#include <ucode/vm.h>

static uc_function_t *funcs[__U_MAX];

static uc_vm_t vm;
static uc_parse_config_t config = {
        .strict_declarations = false,
        .lstrip_blocks = true,
        .trim_blocks = true
};

static int
ucode_run(int function, void (*cb)(char *, void *), void *priv)
{
	int exit_code = 0;

	if (!funcs[function]) {
		fprintf(stderr, "failed to execute function %d\n", function);
		return -1;
	}

	/* increase the refcount of the function */
	ucv_get(&funcs[function]->header);

	/* execute compiled program function */
	uc_value_t *last_expression_result = NULL;
	int return_code = uc_vm_execute(&vm, funcs[function], &last_expression_result);

	/* handle return status */
	switch (return_code) {
	case STATUS_OK:
		exit_code = 0;

		char *s = ucv_to_string(&vm, last_expression_result);

		//printf("Program finished successfully.\n");
		//printf("Function return value is %s\n", s);
		if (s && strlen(s) && cb)
			cb(s, priv);
		break;

	case STATUS_EXIT:
		exit_code = (int)ucv_int64_get(last_expression_result);

		printf("The invoked program called exit().\n");
		printf("Exit code is %d\n", exit_code);
		break;

	case ERROR_COMPILE:
		exit_code = -1;

		printf("A compilation error occurred while running the program\n");
		break;

	case ERROR_RUNTIME:
		exit_code = -2;

		printf("A runtime error occurred while running the program\n");
		break;
	}

	/* free last expression result */
	ucv_put(last_expression_result);

	/* call the garbage collector */
	ucv_gc(&vm);

	return exit_code;
}

int
ucode_rpc(int function, void (*cb)(char *, void *),
	  struct uc_client *client, char *in)
{
	int exit_code = 0;

	/* add global variables x and y to VM scope */
	if (in)
		ucv_object_add(uc_vm_scope_get(&vm), "_rpc", ucv_string_new(in));
	if (client)
		ucv_object_add(uc_vm_scope_get(&vm), "_CN", ucv_string_new(client->CN));

	exit_code = ucode_run(function, cb, client);

	ucv_object_delete(uc_vm_scope_get(&vm), "_rpc");
	ucv_object_delete(uc_vm_scope_get(&vm), "_CN");

	return exit_code;
}

int
ucode_ubus(int function, void (*cb)(char *, void *),
	   const char *method, char *req)
{
	int exit_code = 0;

	/* add global variables x and y to VM scope */
	ucv_object_add(uc_vm_scope_get(&vm), "method", ucv_string_new(method));
	if (req)
		ucv_object_add(uc_vm_scope_get(&vm), "req", ucv_string_new(req));

	exit_code = ucode_run(function, cb, NULL);

	ucv_object_delete(uc_vm_scope_get(&vm), "method");
	ucv_object_delete(uc_vm_scope_get(&vm), "req");

	return exit_code;
}

static uc_function_t*
ucode_load(const char *file) {
	/* create a source buffer from the given input file */
	uc_source_t *src = uc_source_new_file(file);

	/* check if source file could be opened */
	if (!src) {
		fprintf(stderr, "Unable to open source file %s\n", file);
		return NULL;
	}

	/* compile source buffer into function */
	char *syntax_error = NULL;
	uc_function_t *progfunc = uc_compile(&config, src, &syntax_error);

	/* release source buffer */
	uc_source_put(src);

	/* check if compilation failed */
	if (!progfunc)
		fprintf(stderr, "Failed to compile program: %s\n", syntax_error);

	return progfunc;
}

static uc_value_t *
uc_client_send(uc_vm_t *vm, size_t nargs)
{
	uc_value_t *serial = uc_fn_arg(0);
	uc_value_t *msg = uc_fn_arg(1);
	uc_stringbuf_t *buf;

	if (ucv_type(serial) != UC_STRING || ucv_type(msg) != UC_OBJECT)
		return ucv_int64_new(-1);

	buf = ucv_stringbuf_new();

	/* reserve headroom for LWS */
	printbuf_memset(buf, 0, 0, LWS_PRE);

	/* serialize message object as JSON into buffer */
	ucv_to_stringbuf(vm, buf, msg, true);

	ws_client_send(ucv_string_get(serial),
	               &buf->buf[LWS_PRE],
	               printbuf_length(buf) - LWS_PRE);

	printbuf_free(buf);

	return ucv_int64_new(0);
}

int
ucode_init(void)
{
	funcs[U_INIT] = ucode_load("init.uc");
	funcs[U_DISPATCH] = ucode_load("dispatch.uc");
	funcs[U_DISCONNECT] = ucode_load("disconnect.uc");
	funcs[U_UBUS] = ucode_load("ubus.uc");

	/* initialize VM context */
	uc_vm_init(&vm, &config);

	/* load standard library into global VM scope */
	uc_stdlib_load(uc_vm_scope_get(&vm));
	uc_function_register(uc_vm_scope_get(&vm), "client_send", uc_client_send);

	ucode_run(U_INIT, NULL, NULL);

	return 0;
}
