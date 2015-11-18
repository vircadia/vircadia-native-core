var totalTime = 0;
var unsubscribedForTime = 0;
var subscribedForTime = 0;
var subscribed = false;
var SWITCH_SUBSCRIPTION_TIME = 10;
Script.update.connect(function (deltaTime) {
    var channel = "example";
    totalTime += deltaTime;
    if (!subscribed) {
        unsubscribedForTime += deltaTime;
    } else {
        subscribedForTime += deltaTime;
    }

    // if we've been unsubscribed for SWITCH_SUBSCRIPTION_TIME seconds, subscribe
    if (!subscribed && unsubscribedForTime > SWITCH_SUBSCRIPTION_TIME) {
        print("---- subscribing ----");
        Messages.subscribe(channel);
        subscribed = true;
        subscribedForTime = 0;
    }

    // if we've been subscribed for SWITCH_SUBSCRIPTION_TIME seconds, unsubscribe
    if (subscribed && subscribedForTime > SWITCH_SUBSCRIPTION_TIME) {
        print("---- unsubscribing ----");
        Messages.unsubscribe(channel);
        subscribed = false;
        unsubscribedForTime = 0;
    }

    // Even if not subscribed, still publish
    var message = "update() deltaTime:" + deltaTime;
    Messages.sendMessage(channel, message);
});


Messages.messageReceived.connect(function (channel, message, senderID) {
    print("message received on channel:" + channel + ", message:" + message + ", senderID:" + senderID);
});