(function() {
    var _this;

    function ResetGameButton() {
        _this = this;
    }

    ResetGameButton.prototype = {
        preload: function(id) {
            _this.entityID = id;
        },
        clickDownOnEntity: function() {
            _this.resetGame();
        },
        startNearTrigger: function() {
            _this.resetGame();
        },
        startFarTrigger: function() {},
        resetGame: function() {
            print('reset game button calling resetGame');
            Entities.callEntityMethod(Entities.getEntityProperties(_this.entityID, ['parentID']).parentID, 'resetGame');
        }
    };
    return new ResetGameButton();
});
