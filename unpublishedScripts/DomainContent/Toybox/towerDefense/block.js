var BLOCK_MODEL_URL = Script.resolvePath("assets/block.fbx");
var BLOCK_DIMENSIONS = {
    x: 1,
    y: 1,
    z: 1
};
var BLOCK_LIFETIME = 120;

getBlockProperties = function() {
    return {
        type: "Model",
        name: "TD.block",
        modelURL: BLOCK_MODEL_URL,
        shapeType: "compound",
        //compoundShapeURL: BLOCK_COMPOUND_SHAPE_URL,
        dimensions: BLOCK_DIMENSIONS,
        dynamic: 1,
        gravity: { x: 0.0, y: -9.8, z: 0.0 },
        collisionsWillMove: 1,
        lifetime: BLOCK_LIFETIME,
        script: Script.resolvePath("destructibleEntity.js")
    };
}
