var HOST = "localhost:5000"
// var HOST = "desolate-bastion-1742.herokuapp.com";
var client = new WebSocket("ws://" + HOST);
var score = 1;
// var username = GlobalServices.username;
var username = "rand " + Math.floor(Math.random() * 100);
client.onerror = function() {
    print("CONNECTION ERROR");
}
client.onopen = function() {
    print("Web Socket client connected");

    function sendMessage() {
        if(client.readyState === client.OPEN) {
            
            Script.setTimeout(sendMessage, 10000);
            client.send(JSON.stringify({username: username, score: score}))
            score++;
        }
    }
    sendMessage();
}