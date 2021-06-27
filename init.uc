{%
let fs = require("fs");

log = {
	info: function(serial, msg) {
		printf("INFO: %s%s\n",
		       serial ? serial + " " : '',
		       msg);
	},

	warning: function(serial, msg) {
		printf("WARN: %s%s\n",
		       serial ? serial + " " : '',
		       msg);
	},

	error: function(serial, msg) {
		printf("ERR: %s%s\n",
		       serial ? serial + " " : '',
		       msg);
	},

	debug: function(serial, msg) {
		printf("DBG: %s%s\n",
		       serial ? serial + " " : '',
		       msg);
	},
},

jsonrpc = {
	/* the id counter of commands */
	id: 1,

	pending: {},

	new: function(serial, method, params) {
		let id = this.id++;

		/* track rpc calls that we sent */
		this.pending[id] = {
			time: time(),
			serial,
			method,
		};

		/* send the configure call */
		return {
			jsonrpc: "2.0",
			method,
			id,
			params,
		};
	},

	result: function(serial, rpc) {
		/* see if we know this command */
		if (!rpc.id || !length(this.pending[rpc.id]))
			return;
		if (!rpc.result || !rpc.result.status || !rpc.result.status.error) {
			log.debug(serial, sprintf("execute (%d) %s", rpc.id,
				this.pending[rpc.id].method));
			return;
		}

		if (rpc.result.status.rejected)
			/* log the rejects */
			log.warning(serial, sprintf("execute (%d) %s - rejected: %s", rpc.id,
				    this.pending[rpc.id].method, rpc.result.status.rejected));
		else
			/* log the error */
			log.error(serial, sprintf("failed to execute (%d) %s - %s", rpc.id,
				  this.pending[rpc.id].method, rpc.result.status.text));

		delete this.pending[rpc.id];

	},
};

devices = {
	devices: {},

	get: function(serial) {
		return this.devices[serial];
	},

	get_all: function() {
		return this.devices;
	},

	connected: function(serial) {
		return !length(this.devices[serial]);
	},

	connect: function(serial, rpc) {
		/* store the information that the device has sent */
		if (!length(this.devices[serial]))
			this.devices[serial] = {
				info: {},
				config: {},
				state: [],
				healthcheck: [],
			};
		delete this.devices[serial].info.disconnected;
		this.devices[serial].info.connected = time();
		this.devices[serial].capabilities = rpc.params.capabilities;
		this.devices[serial].serial = rpc.params.serial;
		this.devices[serial].info.firmware = rpc.params.firmware;
		this.devices[serial].info.uuid = rpc.params.uuid;
		if (rpc.params.capabilities)
			this.devices[serial].info.model = rpc.params.capabilities.model;

		/* load the latest known config for the device or load the default config */
		for (let name in [
				"/etc/ucentral/devices/" + serial + "/config.json",
				"/etc/ucentral/devices/default.json" ]) {
			let cfg_file = fs.open(name);
			if (!cfg_file)
				continue;

			this.devices[serial].config = json(cfg_file.read("all"));
			cfg_file.close();
			break;
		}
		log.debug(serial, "connected");
	},

	disconnect: function(serial) {
		this.devices[serial].info.disconnected= time();
	},

	state: function(serial, rpc) {
		let device = this.get(serial);

		/* verify that we were actually sent state info */
		if (!rpc.params || !rpc.params.state)
			return;

		/* store the state */
		push(device.state, {
				time: time(),
				state: rpc.params.state
			});
		log.debug(serial, "received state");
	},

	healthcheck: function(serial, rpc) {
		let device = this.get(serial);

		/* verify that we were actually sent a healthcheck */
		if (!rpc.params || !rpc.params.sanity)
			return;

		let health = {
			sanity: rpc.params.sanity,
			data: rpc.params.data,
		};

		/* store the healthcheck */
		push(device.healthcheck, {
				time: time(),
				health,
			});
		log.debug(serial, "received healthcheck");
	},
};

command = {
	reboot: function() {

	},

	upgrade: function() {

	},

	factory: function() {

	},

	leds: function() {

	},

	perform: function() {

	},

	trace: function() {

	},

	wifiscan: function() {

	},

	leds: function() {

	},
};

dispatch = {
	call: function(serial, rpc) {
		/* is this a result message */
		if (length(rpc.result))
			return this.result(serial, rpc);

		/* is the a normal message */
		if (index([
				"connect", "state", "healthcheck",
			  ],
			  rpc.method) < 0)
			return;

		return this[rpc.method](serial, rpc);
	},

	connect: function(serial, rpc) {
		let device = devices.get(serial);

		/* do we have a newer config for the device */
		if (device.config.uuid <= device.info.uuid) {
			log.info(serial, sprintf("connected with uuid %s", device.info.uuid));
			return;
		}

		/* assemble the reply message containing the updated configuration */
		let params = {
			config: device.config,
			uuid: device.config.uuid,
			serial,
		};

		log.debug(serial, sprintf("connected with uuid %s, sending %s",
					  device.info.uuid, device.config.uuid));

		device.info.uuid = device.config.uuid;

		return jsonrpc.new(serial, "configure", params);
	},

	result: function(serial, rpc) {
		/* device sent a reply to some command */
		jsonrpc.result(serial, rpc);
	},

	state: function(serial, rpc) {
		/* device sent a state report */
		devices.state(serial, rpc);
	},

	healthcheck: function(serial, rpc) {
		/* device sent a state report */
		devices.healthcheck(serial, rpc);
	},
};

global = {
	command,
	dispatch,
	devices,
	log,
	rpc,
};
%}
