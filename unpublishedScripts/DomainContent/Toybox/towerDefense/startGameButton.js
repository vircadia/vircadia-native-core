(function() {
    var itemID;

    this.preload = function(entityID) {
        itemID = entityID;
    }

	function signalAC() {
        print("Button pressed");
        var userData = Entities.getEntityProperties(itemID, ["userData"]).userData;
        print("Sending message to: ", JSON.parse(userData).gameChannel);
        Messages.sendMessage(JSON.parse(userData).gameChannel, JSON.stringify({
            type: 'start-game'
        }));
    }

    this.startNearTrigger = signalAC;
    this.startFarTrigger = signalAC;
    this.clickDownOnEntity = signalAC;
})
