(function() {
    var TutorialStartZone = function() {
    };

    TutorialStartZone.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
        },
        enterEntity: function() {
            // send message to outer zone
            print("ENTERED THE TUTORIAL START AREA");
            var parentID = Entities.getEntityProperties(this.entityID, 'parentID').parentID;
            print("HERE", parentID);
            if (parentID) {
                print("HERE2");
                Entities.callEntityMethod(parentID, 'start');
                print("HERE4");
            } else {
                print("HERE3");
                print("ERROR: No parent id found on tutorial start zone");
            }
        },
        leaveEntity: function() {
            print("EXITED THE TUTORIAL START AREA");
        }
    };

    return new TutorialStartZone();
});
