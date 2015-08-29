$(function(){

var FADE_TIME = 150; // ms

// Initialize varibles
var $window = $(window);
var $messages = $('.messages'); // Messages area
var $inputMessage = $('.inputMessage'); // Input message input box


// Prompt for setting a username
var connected = true;

var socket = io();

// Initialize varibles
var $window = $(window);
// Prompt for setting a username

socket.emit('new connection', {pass : $("#pass").html()});
		
		// Send a message
function sendMessage() {
	var message = {};
	message.sw = {};
			//collect data from inputs
 for (var i = 0; i < 7; i++) {
		message.sw[i] = $("#sw"+i).val(); 
 }
	if (message) {
		//message.pass = $("#pass").html();
		socket.emit('new message', message);
	}

}



	// Whenever the server emits 'user joined', log it in the chat body
socket.on('data received', function (data) {
	//printLog(data);
	//var results = JSON.parse(data);
	console.log(data);
	/*for (var i; i < 8; i++) {
		console.log(results.amperemeter[i]);
		$("#amp" + i).val(results.amperemeter[i]);
	}
	for (var i; i < 2; i++) {
		console.log(results.voltmeter[i]);
		$("#volt" + i).val(results.voltmeter[i]);
	}*/

});


function addMessageElement (el, options) {
	var $el = $(el);

	// Setup default options
	if (!options) {
	  options = {};
	}
	if (typeof options.fade === 'undefined') {
	  options.fade = true;
	}
	if (typeof options.prepend === 'undefined') {
	  options.prepend = false;
	}

	// Apply options
	if (options.fade) {
	  $el.hide().fadeIn(FADE_TIME);
	}
	if (options.prepend) {
	  $messages.prepend($el);
	} else {
	  $messages.append($el);
	}
	$messages[0].scrollTop = $messages[0].scrollHeight;
}


// Sends a chat message
function sendMessage () {
	var x;
	var message = {};
	message.sw = {};


	for(x=0; x<8; x++){
		message.sw[x]=$("#sw"+x).val();	
	}

	// if there is a non-empty message and a socket connection
	if (message) {
	  $inputMessage.val('');
	  var time =  new Date();
	  printLog({
		date: time,
		message: message
	  });
	  message.date = time;
	  message.pass = "minhasenha334" ;
	  // tell server to execute 'new message' and send along one parameter
	  socket.emit('new message', message);
	}
}

// Adds the visual chat message to the message list
function printLog (m, options) {
// Don't fade the message in if there is an 'X was typing'
options = options || {};

var $dateDiv = $('<span class="date"/>').text(m.date);
var $messageBodyDiv = $('<span class="messageBody">').text(m.message.toString());

var $messageDiv = $('<li class="message"/>')
  .data('date', m.date).append($dateDiv, $messageBodyDiv);

addMessageElement($messageDiv, options);
}


// Adds a message element to the messages and scrolls to the bottom
// el - The element to add as a message
// options.fade - If the element should fade-in (default = true)
// options.prepend - If the element should prepend
//   all other messages (default = false)



// Keyboard events

$window.keydown(function (event) {
// Auto-focus the current input when a key is typed
if (!(event.ctrlKey || event.metaKey || event.altKey)) {
  //$currentInput.focus();
}

// When the client hits ENTER on their keyboard
if (event.which === 13) {
	sendMessage();
}

});



// Click events
		// Eventos nas chaves
$('.chave').click(function () {
var key = $(this).attr("id");
	if ($(this).hasClass("on")) {
		$(this).attr('class', 'chave off');

	} else if ($(this).hasClass("off")) {
		$(this).attr('class', 'chave on');
	}
	if (connected) {
		sendMessage();
	}
});

// Socket events


});