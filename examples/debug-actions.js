//
//
//


function printData(data) {
    var str = '';
    for (var key in data) {
        if (typeof data[key] == 'object') str += key + printData(data[key]) + ' ';
        else str += key + ':' + data[key] + ' ';
    }
    return str;
};

function printActions(deltaTime) {
    var printed = false;
    var ids = Entities.findEntities(MyAvatar.position, 10);
    for (var i = 0; i < ids.length; i++) {
        var entityID = ids[i];
        var actionIDs = Entities.getActionIDs(entityID);
        if (actionIDs.length > 0) {
            var props = Entities.getEntityProperties(entityID);
            var output = props.name + " ";
            for (var actionIndex = 0; actionIndex < actionIDs.length; actionIndex++) {
                actionID = actionIDs[actionIndex];
                actionArguments = Entities.getActionArguments(entityID, actionID);
                // output += actionArguments['type'];
                output += "(" + printData(actionArguments) + ") ";
            }
            if (!printed) {
                print("---------");
                printed = true;
            }
            print(output);
        }
    }
}


Script.setInterval(printActions, 5000);

// Script.update.connect(printActions);
