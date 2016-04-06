(function() {

    var _this;

    function BaseBox() {
        _this = this;
        return this;
    }

    BaseBox.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
        },
        continueNearGrab: function() {
            var myProps = Entities.getEntityProperties(_this.entityID)
            var results = Entities.findEntities(myProps.position, 2);
            var lid = null;
            results.forEach(function(result) {
                var resultProps = Entities.getEntityProperties(result);
                if (resultProps.name.indexOf('music_box_lid') > -1) {
                    lid = result;
                }
            });
            if (lid === null) {
                return;
            } else {
                Entities.callEntityMethod(lid, 'updateHatRotation');
                Entities.callEntityMethod(lid, 'updateKeyRotation');
            }
        }
    }

    return new BaseBox
})