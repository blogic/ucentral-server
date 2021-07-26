{%
let calls = {
	connected: function() {
		let devs = devices.get_all();
		let ret = {};
		for (let dev in devs)
			ret[dev] = devs[dev].info;
		return ret;
	},

	devices: function() {
		return devices.get_all();
	},

	health: function() {
		if (!req || !req.serial)
			return;
		let dev = devices.get(req.serial);
		return { health: dev ? dev.healthcheck : [] };
	},

	state: function() {
		if (!req || !req.serial)
			return;
		let dev = devices.get(req.serial);
		return { state: dev ? dev.state : [] };
	},

	blink: function() {
		if (!req || !req.serial)
			return;
		let params = {
			serial: req.serial,
			pattern: 'blink',
		};
		let cmd = jsonrpc.new(serial, 'leds', params);
		client_send(req.serial, cmd);
	},

	reboot: function() {
		if (!req || !req.serial)
			return;
		let params = {
			serial: req.serial,
		};
		let cmd = jsonrpc.new(serial, 'reboot', params);
		client_send(req.serial, cmd);
	},

	request: function() {
		if (!req || !req.serial)
			return;
		let params = {
			serial: req.serial,
			message: req.type || 'state',
			request_uuid: ""
		};
		let cmd = jsonrpc.new(serial, 'request', params);
		client_send(req.serial, cmd);
	},


};

if (req)
	req = json(req);

return calls[method]();
%}
