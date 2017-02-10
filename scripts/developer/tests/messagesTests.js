
var channelName = "com.highfidelity.example.dataMessages";

Messages.subscribe(channelName);

//messageReceived(QString channel, QString message, QUuid senderUUID, bool localOnly);
Messages.messageReceived.connect(function(channel, message, sender, local) {
    print("message recieved on ", channel, " message:", message, " from:", sender, " local:", local);
});

Messages.dataReceived.connect(function(channel, data, sender, local) {
    var int8data = new Int8Array(data);
    var dataAsString = "";
    for (var i = 0; i < int8data.length; i++) {
        if (i > 0) {
            dataAsString += ", ";
        }
        dataAsString += int8data[i];
    }
    print("data recieved on ", channel, " from:", sender, " local:", local, "length of data:", int8data.length, " data:", dataAsString);
});

var counter = 0;
Script.update.connect(function(){
    counter++;
    if (counter == 100) {
        Messages.sendMessage(channelName, "foo");
    } else if (counter == 200) {
        var data = new Int8Array([0,1,10,2,20,3,30]);
        print("about to call sendData() data.length:", data.length);
        Messages.sendData(channelName, data.buffer);
        counter = 0;
    }
});

Script.scriptEnding.connect(function(){
    Messages.unsubscribe(channelName);
});
