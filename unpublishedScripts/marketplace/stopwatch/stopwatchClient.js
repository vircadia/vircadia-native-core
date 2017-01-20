(function() {
    this.equipped = false;
    this.preload = function(entityID) {
        this.entityID = entityID;
        this.messageChannel = "STOPWATCH-" + entityID;
        print("In stopwatch client", this.messageChannel);
    };
    this.startEquip = function(entityID, args) {
        this.equipped = true;
    };
    this.continueEquip = function(entityID, args) {
        var triggerValue = Controller.getValue(args[0] === 'left' ? Controller.Standard.LT : Controller.Standard.RT);
        if (triggerValue > 0.5) {
            Messages.sendMessage(messageChannel, 'click');
        }
    };
    this.releaseEquip = function(entityID, args) {
        this.equipped = false;
    };
});
