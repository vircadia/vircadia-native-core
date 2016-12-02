(function() {
    var itemID;

    this.preload = function(entityID) {
        itemID = entityID;
    }

	function signalAC() {
        var userData = Entities.getEntityProperties(itemID, ["userData"]).userData;
        Messages.sendMessage(JSON.parse(userData).gameChannel, "START");
    }

    this.startNearTrigger = signalAC;
    this.startFarTrigger = signalAC;
    this.clickDownOnEntity = signalAC;
})