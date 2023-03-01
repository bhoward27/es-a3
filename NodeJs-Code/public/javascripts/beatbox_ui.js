// Code provided by Dr. Fraser
"use strict";
// Client-side interactions with the browser.

// Make connection to server when web page is fully loaded.
var socket = io.connect();
var modes = ["none", "standard", "alternate"]

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
	$('#cymbal').click(function(){
		sendCommandViaUDP("play cymbal");
	});
	$('#clave').click(function(){
		sendCommandViaUDP("play clave");
	});
	$('#tom-drum').click(function(){
		sendCommandViaUDP("play tom-drum");
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
		flag = false;
	});
	socket.on('volumeReply', function(result) {
		flag = false;
		$('#volumeid').val(result);
	});
	socket.on('tempoReply', function(result) {
		flag = false;
		$('#tempoid').val(result);
	});
	socket.on('modeReply', function(result) {
		flag = false;
		$('#modeid').text(result);
	});
	socket.on('updateReply', function(result) {
		flag = false;
		result = result.split(' ');
		$('#modeid').text(modes[result[1]]);
		$('#tempoid').val(result[2]);
		$('#volumeid').val(result[3]);
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
setInterval(sendFieldUpdateCommand, 250);

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
