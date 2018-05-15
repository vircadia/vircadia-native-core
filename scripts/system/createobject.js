(function(){
    var button;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

    function createEntity(description, position, parent) {
        var entity = Entities.addEntity({
                type: "Sphere",
                position: position,
                dimensions: Vec3.HALF,
                dynamic: true,
                collisionless: true,
                parentID: parent,
                lifetime: 300  // Delete after 5 minutes.
            });
        //print(description + ": " + entity);
        return entity;
    }

    function createBabies() {
        var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 2, z: -5}));
        //var root = createEntity("Root", position, Uuid.NULL);
        var avatarChildren = [];
        var overlayChildren = [];
        var entityChildren = [];
        var avatar = MyAvatar.sessionUUID;
        if (avatar === Uuid.NULL) {
            avatar = MyAvatar.SELF_ID;
        }
        var textToWrite = "Avatar UUID: " + avatar + "\n\n";
        // make five children.
        for(var ctr = 0; ctr < 5; ctr++) {
            var entity = createEntity("AvatarChild" + ctr, Vec3.sum(position, { x: ctr, y: -1, z: ctr }), avatar);
            avatarChildren.push(entity);
            textToWrite += "AvatarChild" + ctr + " UUID: " + entity + "\n";
        }
        var overlay = Overlays.addOverlay("cube", {
            position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
            rotation: MyAvatar.orientation,
            dimensions: { x: 0.3, y: 0.3, z: 0.3 },
            solid: true
        });
        textToWrite += "\nOverlay UUID: " + overlay + "\n\n";
        for(var ctr = 0; ctr < 5; ctr++) {
            var entity = createEntity("OverlayChild" + ctr, Vec3.sum(position, { x: ctr, y: -1, z: ctr }), overlay);
            overlayChildren.push(entity);
            textToWrite += "OverlayChild" + ctr + " UUID: " + entity + "\n";
        }
        var rootEntity = Entities.addEntity({
                type: "Sphere",
                position: position,
                dimensions: Vec3.HALF,
                dynamic: true,
                collisionless: true,
                lifetime: 300  // Delete after 5 minutes.
        });
        textToWrite += "\nEntity UUID: " + rootEntity + "\n\n";
        for(var ctr = 0; ctr < 5; ctr++) {
            var entity = createEntity("EntityChild" + ctr, Vec3.sum(position, { x: ctr, y: -1, z: ctr }), rootEntity);
            entityChildren.push(entity);
            textToWrite += "EntityChild" + ctr + " UUID: " + entity + "\n";
        }
        console.log(textToWrite);
    }

    button = tablet.addButton({
        icon: "icons/tablet-icons/clap-i.svg",
        text: "Create OBJ", 
        sortOrder: 12
    });

    button.clicked.connect(createBabies);

    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(createBabies);
        if (tablet) {
            tablet.removeButton(button);
        }
    });

/*    var entityID = Entities.addEntity({*/
    //type: "Box",
    //position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
    //dimensions: { x: 0.5, y: 0.5, z: 0.5 },
    //dynamic: true,
    //collisionless: false,
    //userData: "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }",
    //lifetime: 300  // Delete after 5 minutes.
    //});

    /*var actionID = Entities.addAction("slider", entityID, {*/
      //axis: { x: 0, y: 1, z: 0 },
      //linearLow: 0,
      //linearHigh: 0.6
    /*});*/
}());
