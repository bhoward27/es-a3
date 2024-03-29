// Code provided by Dr. Fraser
"use strict";
/*
 * Respond to commands over a websocket to relay UDP commands to a local program
 */

var socketio = require('socket.io');
var io;

var dgram = require('dgram');

exports.listen = function(server) {
	io = socketio.listen(server);
	io.set('log level 1');

	io.sockets.on('connection', function(socket) {
		handleCommand(socket);
	});
};

function handleCommand(socket) {
	// Pased string of comamnd to relay
	socket.on('daUdpCommand', function(data) {
		console.log('daUdpCommand command: ' + data);

		// Info for connecting to the local process via UDP
		var PORT = 12345;
		var HOST = '127.0.0.1';
		var buffer = new Buffer(data);

		var client = dgram.createSocket('udp4');
		client.send(buffer, 0, buffer.length, PORT, HOST, function(err, bytes) {
			if (err) 
				throw err;
			console.log('UDP message sent to ' + HOST +':'+ PORT);
		});

		client.on('listening', function () {
			var address = client.address();
			console.log('UDP Client: listening on ' + address.address + ":" + address.port);
		});

		var flag = true;
		// Handle an incoming message over the UDP from the local application.
		client.on('message', function (message, remote) {
			console.log("UDP Client: message Rx" + remote.address + ':' + remote.port +' - ' + message);

			var reply = message.toString('utf8')
			// Learned how to compare part of string from this link: https://stackoverflow.com/questions/13833944/compare-part-of-string-in-javascript
			if (reply.includes("Device")) {
				socket.emit('upTimeReply', reply);
			} else if (reply.includes("volume")) {
				socket.emit('volumeReply', reply.split(' ')[1]);
			} else if (reply.includes("tempo")) {
				socket.emit('tempoReply', reply.split(' ')[1]);
			} else if (reply.includes("mode")) {
				socket.emit('modeReply', reply.split(' ')[1]);
			} else if (reply.includes("update")) {
				socket.emit('updateReply', reply);
			}
			else {
				socket.emit('commandReply', reply);
			}
			
			client.close();
			flag = false;
		});

		setTimeout(function () {
			if (flag) {
				socket.emit('bbgNotRunning', "BBG C++ application is not responding, please check that it is running");
			}
			else {
				socket.emit('bbgRunning', 'BBG is running');
			}
		}, 1000);

		client.on("UDP Client: close", function() {
			console.log("closed");
		});
		client.on("UDP Client: error", function(err) {
			console.log("error: ",err);
		});
	});
};