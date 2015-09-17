BlockManager = function(position) {
    this.position = position;
    this.spawned = false;
    this.blocks = [];

    this.reset = function() {
        if (this.spawned) {
            this.clearBlocks();
        } else {
            this.createBlocks();
        }

        this.spawned = !this.spawned;
    }

    this.clearBlocks = function() {
        this.blocks.forEach(function(block) {
            Entities.deleteEntity(block);
        });
        this.blocks = [];
    }

    this.createBlocks = function() {

        HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
        var modelUrl = HIFI_PUBLIC_BUCKET + 'marketplace/hificontent/Games/blocks/block.fbx';

        var BASE_DIMENSIONS = Vec3.multiply({
            x: 0.2,
            y: 0.1,
            z: 0.8
        }, 0.2);
        var NUM_BLOCKS = 4;

        for (var i = 0; i < NUM_BLOCKS; i++) {
            var block = Entities.addEntity({
                type: "Model",
                modelURL: modelUrl,
                position: Vec3.sum(this.position, {x: 0, y: i/5, z:0}),
                shapeType: 'box',
                name: "block",
                dimensions: Vec3.sum(BASE_DIMENSIONS, {
                    x: Math.random() / 10,
                    y: Math.random() / 10,
                    z: Math.random() / 10
                }),
                collisionsWillMove: true,
                gravity: {
                    x: 0,
                    y: -2.5,
                    z: 0
                },
                velocity: {
                    x: 0,
                    y: -.01,
                    z: 0
                }

            });
            this.blocks.push(block);
        }

    }
}