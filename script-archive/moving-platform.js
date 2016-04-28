var platform;

function init() {
    platform = Entities.addEntity({
        name: "platform",
        type: "Box",
        position: { x: 0, y: 0, z: 0 },
        dimensions: { x: 10, y: 2, z: 10 },
        color: { red: 0, green: 0, blue: 255 },
        gravity: { x: 0, y: 0, z: 0 },
        visible: true,
        locked: false,
        lifetime: 6000,
        velocity: { x: 1.0, y: 0, z: 0 },
        damping: 0,
        isDynamic: false
    });
    if (platform) {
        if (MyAvatar.getParentID() != platform) {
            MyAvatar.position = { x: 0, y: 3, z: 0 };
            MyAvatar.setParentID(platform);
        }
    }
}

function update(dt) {

}

function shutdown() {
    if (platform) {
        MyAvatar.setParentID(0);
        Entities.deleteEntity(platform);
    }
}

Script.setTimeout(init, 3000);

Script.update.connect(update);
Script.scriptEnding.connect(shutdown);
