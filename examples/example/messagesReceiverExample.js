print("---- subscribing ----");
Messages.subscribe(channel);

Messages.messageReceived.connect(function (channel, message, senderID) {
    print("message received on channel:" + channel + ", message:" + message + ", senderID:" + senderID);
});

