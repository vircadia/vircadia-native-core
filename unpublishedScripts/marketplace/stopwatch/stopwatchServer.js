(function() {
    this.equipped = false;
    this.isActive = false;

    this.secondsEntityID = null;
    this.minutesEntityID = null;

    this.preload = function(entityID) {
        print("Stopwatch Server Preload");
        this.entityID = entityID;
        this.messageChannel = "STOPWATCH-" + entityID;

        var userData = Entities.getEntityProperties(this.entityID, 'userData').userData;
        var data = JSON.parse(userData);
        this.secondsEntityID = data.secondsEntityID;
        this.minutesEntityID = data.minutesEntityID;

        this.resetTimer();

        Messages.subscribe(this.messageChannel);
        Messages.messageReceived.connect(this, this.messageReceived);
    };

    this.unload = function() {
        print("Stopwatch Server Unload");
        Messages.unsubscribe(this.messageChannel);
        Messages.messageReceived.disconnect(this, this.messageReceived);
    };

    this.resetTimer = function() {
        Entities.editEntity(this.secondsEntityID, {
            rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
            angularVelocity: { x: 0, y: 0, z: 0 },
        });
        this.isActive = false;
    };
    this.startTimer = function() {
        Entities.editEntity(this.secondsEntityID, {
            angularVelocity: { x: 0, y: -6, z: 0 },
        });
        this.isActive = true;
    };

    this.messageReceived = function(channel, sender, message) {
        print("Message received", channel, sender, message); 
        if (channel == this.messageChannel && message === 'click') {
            if (this.isActive) {
                resetTimer();
            } else {
                startTimer();
            }
        }
    };
});
