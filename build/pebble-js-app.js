var messageArray;
var payload;
var doneMetadata = false;
var lastPosition;
var sendItemIndex;

var typeOfQuery;

function handlePebbleACKMetadata(e) {
	if (doneMetadata === false) {
		sendItemIndex = 0;
		sendMessageItem(sendItemIndex);
	}
	return;
}

function handlePebbleNACKMetadata(e) {
	Pebble.sendAppMessage(payload, handlePebbleACKMetadata, handlePebbleNACKMetadata);
}


function handlePebbleACKMessageItem(e) {
	sendItemIndex++;
	if (sendItemIndex == messageArray.length) {
		sendItemIndex = 0;
		doneMetadata = true;
		//send done
		payload = {"MSGTYPE_KEY" : 2, "LATITUDE_KEY" : lastPosition.coords.latitude, "LONGITUDE_KEY" : lastPosition.coords.longitude, "MESSAGE_KEY" : messageArray.length};
		Pebble.sendAppMessage(payload, handlePebbleACKMetadata, handlePebbleNACKMetadata);
	} else {
		sendMessageItem(sendItemIndex);
	}
}

function handlePebbleNACKMessageItem(e) {
	sendMessageItem(sendItemIndex);
}

function sendMessageItem(i) {
	var dict = {"MSGTYPE_KEY" : 1, "MESSAGE_KEY" : messageArray[i].message, "ID_KEY" : messageArray[i].messageID, "NUMBEROFLIKES_KEY" : messageArray[i].numberOfLikes, "NUMBEROFCOMMENTS_KEY" : messageArray[i].comments};
	//console.log("send " + messageArray[i].comments);
	Pebble.sendAppMessage(dict, handlePebbleACKMessageItem, handlePebbleNACKMessageItem);
}

function parseAndSendMessages(data) {
	var json = JSON.parse(data);

	messageArray = json.messages;
	if (messageArray.length > 30) {
		messageArray = messageArray.slice(0, 30);
	}


	/*
	for (var i = 0; i < messageArray.length; i++) {
		console.log(messageArray[i].message);
	}
	*/

	doneMetadata = false;
	payload = {"MSGTYPE_KEY" : 0, "LATITUDE_KEY" : lastPosition.coords.latitude * 1000, "LONGITUDE_KEY" : lastPosition.coords.longitude * 1000, "MESSAGE_KEY" : messageArray.length};
	Pebble.sendAppMessage(payload, handlePebbleACKMetadata, handlePebbleNACKMetadata);
}

function HTTPGET(url) {
	var req = new XMLHttpRequest();
	req.open('GET', url, true);
	req.onload = function(e) {
		if (req.readyState == 4 && req.status == 200) {
			if(req.status == 200) {
				parseAndSendMessages(req.responseText);
			} else {
				console.log('Error: ' + req.statusText);
			}
		}
	};
	req.send(null);
}

var locationOptions = {
  enableHighAccuracy: true,
  maximumAge: 10000,
  timeout: 10000
};

function locationSuccess(pos) {
	lastPosition = pos;
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);

	var baseURL = "https://us-east-api.yikyakapi.net/api/";

	var query = "" + baseURL;

	switch(typeOfQuery) {
		case 0:
			query += "getMessages?";
			break;
		case 1:
			query += "getAreaTops?";
			break;
		default:
			console.log("unknown query type " + typeOfQuery);
	}

	var userID = "&userID=5DFC2F7A58C34D2F9A41-F1E69AD3A8A";

	query += "lat=" + pos.coords.latitude + "&long=" + pos.coords.longitude;
	query += userID;

	console.log(query);
	HTTPGET(query);
}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

// Called when JS is ready
Pebble.addEventListener("ready", function(e) {

});

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage", function(e) {
	typeOfQuery = e.payload.MESSAGE_KEY;
	console.log("Received payload: " + e.payload.MESSAGE_KEY);
	navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
});
