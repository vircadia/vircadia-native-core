// this script creates a pile of playable blocks and displays a target arrangement when you trigger it

var BLOCK_RED = {
    url: 'atp:/kineticObjects/blocks/planky_red.fbx',
    dimensions: {
        "x": 0.10000000149011612,
        "y": 0.05000000074505806,
        "z": 0.25
    }
};
var BLOCK_BLUE = {
    url: 'atp:/kineticObjects/blocks/planky_blue.fbx',
    dimensions: {
        "x": 0.05000000074505806,
        "y": 0.05000000074505806,
        "z": 0.25
    },
};
var BLOCK_YELLOW = {
    url: 'atp:/kineticObjects/blocks/planky_yellow.fbx',
    dimensions: {
        "x": 0.029999999329447746,
        "y": 0.05000000074505806,
        "z": 0.25
    }
};
var BLOCK_GREEN = {
    url: 'atp:/kineticObjects/blocks/planky_green.fbx',
    dimensions: {
        "x": 0.10000000149011612,
        "y": 0.10000000149011612,
        "z": 0.25
    },
};
var BLOCK_NATURAL = {
    url: "atp:/kineticObjects/blocks/planky_natural.fbx",
    dimensions: {
        "x": 0.05,
        "y": 0.05,
        "z": 0.05
    }
};

var blocks = [
    BLOCK_RED, BLOCK_BLUE, BLOCK_YELLOW, BLOCK_GREEN, BLOCK_SMALL_CUBE
];

var arrangements = [{
    name: 'tall',
    blocks: [BLOCK_GREEN, BLOCK_GREEN, BLOCK_GREEN, ],
    target: Script.resolvePath("arrangement1.json")
}, {
    name: 'ostrich',
    blocks: [BLOCK_RED, BLOCK_RED, BLOCK_GREEN, BLOCK_YELLOW, BLOCK_NATURAL],
    target: Script.resolvePath("arrangement2.json")
}, {
    name: 'froglog',
    blocks: [BLOCK_GREEN, BLOCK_GREEN, BLOCK_GREEN, BLOCK_NATURAL, BLOCK_NATURAL, BLOCK_NATURAL, BLOCK_NATURAL, BLOCK_NATURAL, BLOCK_NATURAL],
    target: Script.resolvePath("arrangement3.json")
}, {
    name: 'allOneLeg',
    blocks: [BLOCK_RED, BLOCK_GREEN, BLOCK_YELLOW, BLOCK_BLUE, BLOCK_BLUE, BLOCK_NATURAL],
    target: Script.resolvePath("arrangement4.json")
}, {
    name: 'threeppl',
    blocks: [BLOCK_BLUE BLOCK_YELLOW, BLOCK_BLUE, BLOCK_NATURAL, BLOCK_NATURAL, BLOCK_NATURAL],
    target: Script.resolvePath("arrangement5.json")
}, {
    name: 'dominoes',
    blocks: [BLOCK_RED, BLOCK_YELLOW, BLOCK_YELLOW, BLOCK_YELLOW, BLOCK_YELLOW, BLOCK_YELLOW, BLOCK_YELLOW],
    target: Script.resolvePath("arrangement6.json")
}]

var PLAYABLE_BLOCKS_POSITION = {
    x: 1097.6,
    y: 460.5,
    z: -66.22
};

var TARGET_BLOCKS_POSITION = {
    x: 1096.82,
    y: 460.5,
    z: -67.689
};

(function() {
    //#debug
    print('BLOCK ENTITY SCRIPT')
    var _this;

    function Blocky() {
        _this = this;
    }

    Blocky.prototype = {
        debug: true,
        playableBlocks: [],
        targetBlocks: [],
        preload: function(entityID) {
            print('BLOCKY preload')
            this.entityID = entityID;
        },
        createTargetBlocks: function(arrangement) {
            var created = [];
            print('BLOCKY create target blocks')

            var created = [];
            var success = Clipboard.importEntities(arrangement.target);
            if (success === true) {
                created = Clipboard.pasteEntities(TARGET_BLOCKS_POSITION)
                print('created ' + created);
            }

            this.targetBlocks = created;
            print('BLOCKY TARGET BLOCKS:: ' + this.targetBlocks);
        },
        createPlayableBlocks: function(arrangement) {
            print('BLOCKY creating playable blocks');
            arrangement.blocks.forEach(function(block) {
                var blockProps = {
                    name: "home_model_blocky_block",
                    type: 'Model',
                    collisionSoundURL: "atp:/kineticObjects/blocks/ToyWoodBlock.L.wav",
                    dynamic: true,
                    gravity: {
                        x: 0,
                        y: -9.8,
                        z: 0
                    },
                    userData: JSON.stringify({
                        grabbableKey: {
                            grabbable: true
                        },
                        hifiHomeKey: {
                            reset: true
                        }
                    }),
                    dimensions: block.dimensions,
                    modelURL: block.url,
                    shapeType: 'box',
                    velocity: {
                        x: 0,
                        y: -0.01,
                        z: 0,
                    },
                    position: PLAYABLE_BLOCKS_POSITION
                }
                this.playableBlocks.push(Entities.addEntity(blockProps));

            })
        },
        startNearTrigger: function() {
            print('BLOCKY got a near trigger');
            this.advanceRound();
        },
        advanceRound: function() {
            print('BLOCKY advance round');
            this.cleanup();
            var arrangement = arrangements[Math.floor(Math.random() * arrangements.length)];
            this.createTargetBlocks(arrangement);

            if (this.debug === true) {
                this.debugCreatePlayableBlocks();
            } else {
                this.createPlayableBlocks(arrangement);

            }
        },
        cleanup: function() {
            print('BLOCKY cleanup');
            this.targetBlocks.forEach(function(block) {
                Entities.deleteEntity(block);
            });
            this.playableBlocks.forEach(function(block) {
                Entities.deleteEntity(block);
            });
            this.targetBlocks = [];
            this.playableBlocks = [];
        },
        debugCreatePlayableBlocks: function() {
            print('BLOCKY debug create');
            var howMany = 10;
            var i;
            for (i = 0; i < howMany; i++) {
                var block = blocks[Math.floor(Math.random() * blocks.length)];
                var blockProps = {
                    name: "home_model_blocky_block",
                    type: 'Model',
                    collisionSoundURL: "atp:/kineticObjects/blocks/ToyWoodBlock.L.wav",
                    dynamic: true,
                    gravity: {
                        x: 0,
                        y: -9.8,
                        z: 0
                    },
                    userData: JSON.stringify({
                        grabbableKey: {
                            grabbable: true
                        },
                        hifiHomeKey: {
                            reset: true
                        }
                    }),
                    dimensions: block.dimensions,
                    modelURL: block.url,
                    shapeType: 'box',
                    velocity: {
                        x: 0,
                        y: -0.01,
                        z: 0,
                    },
                    position: PLAYABLE_BLOCKS_POSITION
                }
                this.playableBlocks.push(Entities.addEntity(blockProps));
            }
        },
        unload: function() {
            this.cleanup();
        },
        clickReleaseOnEntity: function() {
            print('BLOCKY click')
            this.startNearTrigger();
        }
    }

    return new Blocky();
})