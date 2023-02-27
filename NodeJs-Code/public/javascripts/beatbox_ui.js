// Code provided by Dr. Fraser
"use strict";
// Client-side interactions with the browser.

// Make connection to server when web page is fully loaded.
var socket = io.connect();
$(document).ready(function() {

	$('#modeNone').click(function(){
		sendCommandViaUDP("modeNone");
	});
	$('#btnCount').click(function(){
		sendCommandViaUDP("count");
	});
	$('#btnLength').click(function(){
		sendCommandViaUDP("length");
	});
	$('#btnHistory').click(function(){
		sendCommandViaUDP("history");
	});
	$('#btnStop').click(function(){
		sendCommandViaUDP("stop");
	});
	
	socket.on('commandReply', function(result) {
		var newDiv = $('<code></code>')
			.text(result)
			.wrapInner("<div></div>");
		$('#messages').append(newDiv);
		$('#messages').scrollTop($('#messages').prop('scrollHeight'));
	});
});

function sendCommandViaUDP(message) {
	socket.emit('daUdpCommand', message);
};

// Learned how to send a command every second from the link below
// https://stackoverflow.com/questions/45752698/periodically-call-node-js-function-every-second
setInterval(sendUpTimeCommandViaUDP, 1000);

function sendUpTimeCommandViaUDP() {
	socket.emit('daUdpCommand', "test");
	socket.on('commandReply', function(result) {
		var newDiv = $('<code></code>')
			.text(result)
			.wrapInner("<div></div>");
		$('#status').html(newDiv);
	});
};
