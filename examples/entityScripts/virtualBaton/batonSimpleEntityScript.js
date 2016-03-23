(function() {
    Script.include("../../libraries/virtualBaton.js");

    var _this = this;


    this.startUpdate = function() {
        Entities.editEntity(_this.batonDebugModel, {visible: true});

    }

    this.maybeClaim = function() {
        if (_this.isBatonOwner === true) {
            _this.isBatonOwner = false;
        }
        Entities.editEntity(_this.batonDebugModel, {visible: false});
        baton.claim(_this.startUpdate, _this.maybeClaim);
    }

    this.preload = function(entityID) {
        _this.entityID = entityID
        _this.batonDebugModel = Entities.addEntity({
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
        _this.colorsToCycle = [{
            red: 200,
            green: 0,
            blue: 0
        }, {
            red: 0,
            green: 200,
            blue: 0
        }, {
            red: 0,
            green: 0,
            blue: 200
        }];
        baton = virtualBaton({
            batonName: "batonSimpleEntityScript:" + _this.entityID
        });
        _this.isBatonOwner = false;
        _this.maybeClaim();

        print("EBL Preload!!");
    }

});