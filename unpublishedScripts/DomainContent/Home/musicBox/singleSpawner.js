    var musicBoxPath = Script.resolvePath('wrapper.js?' + Math.random());
    Script.include(musicBoxPath);
    var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
        x: 0,
        y: 0.5,
        z: 0
    }), Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));
    var musicBox = new HomeMusicBox(center, {
        x: 0,
        y: 0,
        z: 0
    });

    Script.scriptEnding.connect(function() {
        musicBox.cleanup()
    })