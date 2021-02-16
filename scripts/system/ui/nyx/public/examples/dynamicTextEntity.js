(function () {
    var NyxAlpha1 = Script.require('../nyx-helpers.js?12dsadsddsadssdassaddseras3');

    var _entityID;
    
    function onEntityMenuTriggered(triggeredEntityID, command, data) {
        if (data.name === ':)' && triggeredEntityID === _entityID) {
            Entities.editEntity(_entityID, { 
                text: data.value
            });
        }
    }

    this.preload = function (entityID) {
        _entityID = entityID;

        var initialProps = Entities.getEntityProperties(_entityID, ['text']);
        
        NyxAlpha1.registerWithEntityMenu(entityID, [
            {
                type: 'textField',
                name: ':)',
                hint: 'just smile!',
                initialValue: initialProps.text
            }
        ]);
        NyxAlpha1.entityMenuTriggered.connect(onEntityMenuTriggered);
    };

    this.unload = function () {
        NyxAlpha1.entityMenuTriggered.disconnect(onEntityMenuTriggered);
    };

});