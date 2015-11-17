var totalTime = 0;
var subscribed = false;
var WAIT_FOR_SUBSCRIPTION_TIME = 10;
function myUpdate(deltaTime) {
    var channel = "example";
    totalTime += deltaTime;

    if (totalTime > WAIT_FOR_SUBSCRIPTION_TIME && !subscribed) {

        print("---- subscribing ----");
        Messages.subscribe(channel);
        subscribed = true;
        Script.update.disconnect(myUpdate);
    }
}

Script.update.connect(myUpdate);

Messages.messageReceived.connect(function (channel, message) {
    print("message received on channel:" + channel + ", message:" + message);
});

