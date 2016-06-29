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
    BLOCK_RED, BLOCK_BLUE, BLOCK_YELLOW, BLOCK_GREEN, BLOCK_NATURAL
];

var arrangements = [{
    name: 'greenhenge',
    blocks: [BLOCK_GREEN, BLOCK_GREEN, BLOCK_YELLOW, BLOCK_YELLOW],
    target: "atp:/blocky/newblocks1.json"
}, {
    name: 'tallstuff',
    blocks: [BLOCK_RED, BLOCK_RED, BLOCK_RED],
    target: "atp:/blocky/newblocks2.json"
}, {
    name: 'ostrich',
    blocks: [BLOCK_RED, BLOCK_RED, BLOCK_GREEN, BLOCK_YELLOW, BLOCK_NATURAL],
    target: "atp:/blocky/newblocks5.json"
}, {
    name: 'fourppl',
    blocks: [BLOCK_BLUE, BLOCK_BLUE, BLOCK_BLUE, BLOCK_BLUE, BLOCK_NATURAL, BLOCK_NATURAL, BLOCK_NATURAL, BLOCK_NATURAL],
    target: "atp:/blocky/newblocks3.json"
}, {
    name: 'frogguy',
    blocks: [BLOCK_GREEN, BLOCK_GREEN, BLOCK_GREEN, BLOCK_YELLOW, BLOCK_RED, BLOCK_RED, BLOCK_NATURAL, BLOCK_NATURAL],
    target: "atp:/blocky/newblocks4.json"
}, {
    name: 'dominoes',
    blocks: [BLOCK_RED, BLOCK_YELLOW, BLOCK_YELLOW, BLOCK_YELLOW, BLOCK_YELLOW, BLOCK_YELLOW, BLOCK_YELLOW],
    target: "atp:/blocky/arrangement6B.json"
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
//#debug
(function() {

    print('BLOCK ENTITY SCRIPT')
    var _this;

    function Blocky() {
        _this = this;
    }

    Blocky.prototype = {
        busy: false,
        debug: false,
        playableBlocks: [],
        targetBlocks: [],
        preload: function(entityID) {
            print('BLOCKY preload')
            this.entityID = entityID;
            Script.update.connect(_this.update);
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
            print('BLOCKY creating playable blocks' + arrangement.blocks.length);
            arrangement.blocks.forEach(function(block) {
                print('BLOCKY in a block loop')
                var blockProps = {
                    name: "home_model_blocky_block",
                    type: 'Model',
                    collisionSoundURL: "atp:/kineticObjects/blocks/ToyWoodBlock.L.wav",
                    dynamic: true,
                    collisionless: false,
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
                var newBlock = Entities.addEntity(blockProps);
                print('BLOCKY made a playable block' + newBlock)
                _this.playableBlocks.push(newBlock);
                print('BLOCKY pushing it into playable blocks');
            })

            print('BLOCKY after going through playable arrangement')
        },

        startNearTrigger: function() {
            print('BLOCKY got a near trigger');
            this.advanceRound();
        },

        advanceRound: function() {
            print('BLOCKY advance round');
            this.busy = true;
            this.cleanup();
            var arrangement = arrangements[Math.floor(Math.random() * arrangements.length)];
            this.createTargetBlocks(arrangement);

            if (this.debug === true) {
                this.debugCreatePlayableBlocks();
            } else {
                this.createPlayableBlocks(arrangement);

            }
            Script.setTimeout(function() {
                _this.busy = false;
            }, 1000)
        },

        findBlocks: function() {
            var found = [];
            var results = Entities.findEntities(MyAvatar.position, 10);
            results.forEach(function(result) {
                var properties = Entities.getEntityProperties(result);
                print('got result props')
                if (properties.name.indexOf('blocky_block') > -1) {
                    found.push(result);
                }
            });
            return found;
        },

        cleanup: function() {
            print('BLOCKY cleanup');
            var blocks = this.findBlocks();
            print('BLOCKY cleanup2' + blocks.length)
            blocks.forEach(function(block) {
                Entities.deleteEntity(block);
            })
            print('BLOCKY after find and delete')
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
                    dynamic: false,
                    collisionless: true,
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
            Script.update.disconnect(_this.update);
        },

        clickReleaseOnEntity: function() {
            print('BLOCKY click')
            this.startNearTrigger();
        },

        update: function() {
            if (_this.busy === true) {
                return;
            }
            var BEAM_TRIGGER_THRESHOLD = 0.075;

            var BEAM_POSITION = {
                x: 1098.4424,
                y: 460.3090,
                z: -66.2190
            };

            var leftHandPosition = MyAvatar.getLeftPalmPosition();
            var rightHandPosition = MyAvatar.getRightPalmPosition();

            var rightDistance = Vec3.distance(leftHandPosition, BEAM_POSITION)
            var leftDistance = Vec3.distance(rightHandPosition, BEAM_POSITION)

            if (rightDistance < BEAM_TRIGGER_THRESHOLD || leftDistance < BEAM_TRIGGER_THRESHOLD) {
                _this.startNearTrigger();
            }
        }
    }

    return new Blocky();
})