(function(){
    var button;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

    function destroyBabies() {
    var avatarEntityID = 
    for(var ctr = 0; ctr < getChildrenIDs(); ctr++) {
        avatarChildren.append(CreateEntity("Child" + ctr, Vec3.sum(position, { x: ctr, y: -1, z: ctr }), root));
    }}

    button = tablet.addButton({
        icon: "icons/tablet-icons/clap-i.svg",
        text: "Destroy OBJ", 
        sortOrder: 1
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
