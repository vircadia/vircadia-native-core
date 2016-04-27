function avatarRelativePosition(position) {
    return Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, position));
}

ColorCube = function() {};
ColorCube.prototype.NAME = "ColorCube";
ColorCube.prototype.POSITION = { x: 0, y: 0.5, z: -0.5 }; 
ColorCube.prototype.USER_DATA = { ProceduralEntity: {
        version: 2, shaderUrl: Script.resolvePath("colorCube.fs"),
} };

// Clear any previous entities within 50 meters
ColorCube.prototype.clear = function() {
    var ids = Entities.findEntities(MyAvatar.position, 50);
    var that = this;
    ids.forEach(function(id) {
        var properties = Entities.getEntityProperties(id);
        if (properties.name == that.NAME) {
            Entities.deleteEntity(id);
        }
    }, this);
}

ColorCube.prototype.create = function() {
    var that = this;
    var size = HMD.ipd;
    var id = Entities.addEntity({
        type: "Box",
        position: avatarRelativePosition(that.POSITION),
        name: that.NAME,
        color: that.COLOR,
        ignoreCollisions: true,
        dynamic: false,
        dimensions: { x: size, y: size, z: size },
        lifetime: 3600,
        userData: JSON.stringify(that.USER_DATA)
    });
}

var colorCube = new ColorCube();
colorCube.clear();
colorCube.create();
