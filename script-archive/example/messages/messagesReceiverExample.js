print("---- subscribing ----");
Messages.subscribe("example");

Messages.messageReceived.connect(function (channel, message, senderID) {
    print("message received on channel:" + channel + ", message:" + message + ", senderID:" + senderID);
});

