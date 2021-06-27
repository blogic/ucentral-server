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
			return '';
		let dev = devices.get(req.serial);
		return { health: dev ? dev.healthcheck : [] };
	},

	state: function() {
		if (!req || !req.serial)
			return '';
		let dev = devices.get(req.serial);
		return { state: dev ? dev.state : [] };
	}
};

if (req)
	req = json(req);

return calls[method]();
%}
