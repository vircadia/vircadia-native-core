(function() {
    var messageChannel;
    this.preload = function(entityID) {
        this.messageChannel = "STOPWATCH-" + entityID;
    };
    function click() {
        Messages.sendMessage(this.messageChannel, 'click');
    }
    this.startNearTrigger = click;
    this.startFarTrigger = click;
    this.clickDownOnEntity = click;
});
