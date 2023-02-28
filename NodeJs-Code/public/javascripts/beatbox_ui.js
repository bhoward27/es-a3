// Code provided by Dr. Fraser
"use strict";
// Client-side interactions with the browser.

// Make connection to server when web page is fully loaded.
var socket = io.connect();
$(document).ready(function() {

	$('#modeNone').click(function(){
		sendCommandViaUDP("mode none");
	});
	$('#modeRock1').click(function(){
		sendCommandViaUDP("mode rock1");
	});
	$('#modeRock2').click(function(){
		sendCommandViaUDP("mode rock2");
	});
	$('#volumeDown').click(function(){
		sendCommandViaUDP("volume down");
	});
	$('#volumeUp').click(function(){
		sendCommandViaUDP("volume up");
	});
	$('#tempoDown').click(function(){
		sendCommandViaUDP("tempo down");
	});
	$('#tempoUp').click(function(){
		sendCommandViaUDP("tempo up");
	});
	$('#hiHat').click(function(){
		sendCommandViaUDP("play hihat");
	});
	$('#snare').click(function(){
		sendCommandViaUDP("play snare");
	});
	$('#base').click(function(){
		sendCommandViaUDP("play base");
	});
	$('#terminate').click(function(){
		sendCommandViaUDP("terminate");
	});

	socket.on('bbgNotRunning', function(result) {
		if ($('#error-box').is(":hidden")) {
			var newDiv = $('<code></code>')
				.text(result)
				.wrapInner("<div></div>");
			$('#error-box').show();
			$('#error-text').html(newDiv);
		}
	});

	socket.on('bbgRunning', function(result) {
		if ($('#error-box').is(":visible")) {
			$('#error-box').hide();
		}
	});
});

function sendCommandViaUDP(message) {
	socket.emit('daUdpCommand', message);

	var flag = true;
	socket.on('commandReply', function(result) {
		var newDiv = $('<code></code>')
			.text(result)
			.wrapInner("<div></div>");
		flag = false;
	});

	setTimeout(function () {
		if (flag) {
			// Learned how to check if div is visible from this link: https://stackoverflow.com/questions/178325/how-do-i-check-if-an-element-is-hidden-in-jquery
			if ($('#error-box').is(":hidden")) {
				var newDiv = $('<code></code>')
				.text("NodeJs server is no longer responding, please check that it is running")
				.wrapInner("<div></div>");
				$('#error-box').show();
				$('#error-text').html(newDiv);
			}
		}
	}, 2000);
};

// Learned how to send a command every second from the link below
// https://stackoverflow.com/questions/45752698/periodically-call-node-js-function-every-second
setInterval(sendUpTimeCommandViaUDP, 1000);
setInterval(sendFieldUpdateCommand, 500);

function sendUpTimeCommandViaUDP() {
	socket.emit('daUdpCommand', "UpTime");

	var flag = true;
	socket.on('upTimeReply', function(result) {
		var newDiv = $('<code></code>')
			.text(result)
			.wrapInner("<div></div>");
		$('#status').html(newDiv);
		flag = false;
	});

	setTimeout(function () {
		if (flag) {
			// Learned how to check if div is visible from this link: https://stackoverflow.com/questions/178325/how-do-i-check-if-an-element-is-hidden-in-jquery
			if ($('#error-box').is(":hidden")) {
				var newDiv = $('<code></code>')
				.text("NodeJs server is no longer responding, please check that it is running")
				.wrapInner("<div></div>");
				$('#error-box').show();
				$('#error-text').html(newDiv);
			}
		}
		else {
			if ($('#error-box').is(":visible")) {
				$('#error-box').hide();
			}
		}
	}, 2000);
};

function sendFieldUpdateCommand() {
	sendCommandViaUDP("update fields");
}
