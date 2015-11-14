var HOST = "localhost:5000"
    // var HOST = "https://thawing-hamlet-8433.herokuapp.com/";
var client = new WebSocket("ws://" + HOST);
client.onerror - function() {
    console.log("CONNECTION ERROR");
}
print("TESSST");
client.onopen = function() {
    print("Web Socket client connected");

    function sendMessage() {
        // if(client.readyState === client.OPEN) {
            client.send("HEY");
            Script.setTimeout(sendMessage, 1000);
        // }
    }
    sendMessage();
}