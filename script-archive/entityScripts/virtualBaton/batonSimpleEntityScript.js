(function() {
    Script.include("../../libraries/virtualBaton.js");
    Script.include("../../libraries/utils.js");

    var _this = this;


    this.startUpdate = function() {
        print("EBL START UPDATE");
        Entities.editEntity(_this.batonOwnerIndicator, {
            visible: true
        });

        // Change color of box
        Entities.editEntity(_this.entityID, {
            color: randomColor()
        });

        _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
        _this.debugLightProperties.position = Vec3.sum(_this.position, {x: 0, y: 1, z: 0});
        _this.debugLightProperties.color = randomColor();
        var debugLight = Entities.addEntity(_this.debugLightProperties);
        Script.setTimeout(function() {
            Entities.deleteEntity(debugLight);
        }, 500);

    }

    this.maybeClaim = function() {
        print("EBL MAYBE CLAIM");
        if (_this.isBatonOwner === true) {
            _this.isBatonOwner = false;
        }
        Entities.editEntity(_this.batonOwnerIndicator, {
            visible: false
        });
        baton.claim(_this.startUpdate, _this.maybeClaim);
    }

    this.unload = function() {
        print("EBL UNLOAD");
        baton.unload();
        Entities.deleteEntity(_this.batonOwnerIndicator);
    }


    this.preload = function(entityID) {
        print("EBL Preload!!");
        _this.entityID = entityID;
        _this.setupDebugEntities();

        baton = virtualBaton({
            batonName: "batonSimpleEntityScript:" + _this.entityID
        });
        _this.isBatonOwner = false;
        _this.maybeClaim();

    }


    this.setupDebugEntities = function() {
        _this.batonOwnerIndicator = Entities.addEntity({
            type: "Box",
            color: {
                red: 200,
                green: 10,
                blue: 200
            },
            position: Vec3.sum(MyAvatar.position, {
                x: 0,
                y: 1,
                z: 0
            }),
            dimensions: {
                x: 0.5,
                y: 1,
                z: 0
            },
            parentID: MyAvatar.sessionUUID,
            visible: false
        });
    }

    _this.debugLightProperties = {
        type: "Light",
        name: "hifi-baton-light",
        dimensions: {
            x: 10,
            y: 10,
            z: 10
        },
        falloffRadius: 3,
        intensity: 20,
    }

});