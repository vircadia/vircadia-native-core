    var fishTankPath = Script.resolvePath('wrapper.js?' + Math.random());
    Script.include(fishTankPath);
    var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
        x: 0,
        y: 0.5,
        z: 0
    }), Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));
    var fishTank = new FishTank(center, {
        x: 0,
        y: 123,
        z: 0
    });

    Script.scriptEnding.connect(function(){
        fishtank.cleanup()
    })