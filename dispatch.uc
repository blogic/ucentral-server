{%
	let rpc = json(_rpc);

	if (rpc.jsonrpc != "2.0")
		return;

	if (rpc.method == "connect")
		devices.connect(_CN, rpc);

	if (devices.connected(_CN))
		return;

	if (rpc.params && rpc.params.compress_64) {
		delete rpc.params;
		log.error(_CN, "zlib is not supported yet");
	}

	let reply = dispatch.call(_CN, rpc);

	return reply || '';
%}
