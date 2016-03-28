// when you drop a doll and it hits the dressing room platform it transforms into a big version. 
// the small doll is destroyed and a new small doll is put on the shelf


(function() {
    var TRIGGER_DISTANCE = 0.85;
    var TRANSFORMATION_SOUND_URL = '';

    var _this;

    function Transformer() {
        _this = this;
        return this;
    }

    Transformer.prototype = {
        locked: false,
        rotatorBlock: null,
        transformationSound: null,
        preload: function(entityID) {
            print('PRELOAD TRANSFORMER SCRIPT')
            this.entityID = entityID;
            this.initialProperties = Entities.getEntityProperties(entityID);
            this.transformationSound = SoundCache.getSound(TRANSFORMATION_SOUND_URL);
        },

        collisionWithEntity: function(myID, otherID, collisionInfo) {
            var otherProps = Entities.getEntityProperties(otherID);
           
            if (otherProps.name = "hifi-home-dressing-room-transformer-collider" && _this.locked === false) {
                var myProps = Entities.getEntityProperties(myID);
                var distance = Vec3.distance(myProps.position, otherProps.position);
                print('transformer DISTANCE ' + distance)
                if (distance < TRIGGER_DISTANCE) {
                    print('transformer should do magic!!!')
                        // this.playTransformationSound(collisionInfo.contactPoint);
                        // this.createTransformationParticles();
                    _this.findRotatorBlock();
                } else {
                    return;
                }
                this.locked = true;

            } else {
                return;
            }
        },

        playTransformationSound: function(position) {
            print('transformer should play a sound')
            Audio.playSound(_this.transformationSound, {
                position: position,
                volume: 0.5
            });
        },

        createTransformationParticles: function() {
            print('transformer should create particles')
            var particleProps = {};
            Entities.addEntity(particleProps);
        },

        findRotatorBlock: function() {
            print('transformer should find rotator block')
            var myProps = Entities.getEntityProperties(_this.entityID);
            var results = Entities.findEntities(myProps.position, 10);
            results.forEach(function(result) {
                var resultProps = Entities.getEntityProperties(result);
                if (resultProps.name === "hifi-home-dressing-room-rotator-block") {
                    _this.rotatorBlock = result;
                    _this.removeCurrentBigVersion(result);
                }
            });

        },

        removeCurrentBigVersion: function(rotatorBlock) {
            print('transformer should remove big version')
            var myProps = Entities.getEntityProperties(_this.entityID);
            var results = Entities.findEntities(myProps.position, 10);
            results.forEach(function(result) {
                var resultProps = Entities.getEntityProperties(result);
                if (resultProps.name === "hifi-home-dressing-room-big-transformer") {
                    Entities.deleteEntity(result);
                }
            });

            _this.createBigVersion(myProps);

        },

        createBigVersion: function(smallProps) {
            print('transformer should create big version!!')
            print('transformer has rotatorBlock??' + _this.rotatorBlock);
            var rotatorProps = Entities.getEntityProperties(_this.rotatorBlock);
            var bigVersionProps = {
                name: "hifi-home-dressing-room-big-transformer",
                type: 'Model',
                parentID: _this.rotatorBlock,
                modelURL: smallProps.modelURL,
                position: _this.putTransformerOnRotatorBlock(rotatorProps.position),
                rotation: rotatorProps.rotation,
                userData: JSON.stringify({
                    'grabbableKey': {
                        'grabbable': false
                    },
                    'hifiHomeKey': {
                        'reset': true
                    }
                }),
            }

            var bigVersion = Entities.addEntity(bigVersionProps);
            print('transformer created big version: ' + bigVersion)
            _this.putNewVersionOnShelf();
        },

        putTransformerOnRotatorBlock: function(blockPosition) {
            print('transformer should get set on rotator block')
            return blockPosition
        },

        putNewVersionOnShelf: function() {
            print('transformer should out a new version of itself on the shelf')
            var littleVersionProps = Entities.getEntityProperties(_this.entityID);
            delete littleVersionProps.id;
            delete littleVersionProps.created;
            delete littleVersionProps.age;
            delete littleVersionProps.ageAsText;
            delete littleVersionProps.position;
            delete littleVersionProps.rotation;
            delete littleVersionProps.localPosition;
            delete littleVersionProps.localRotation;
            delete littleVersionProps.naturalPosition;
            // delete littleVersionProps.script;

            var userData = JSON.parse(littleVersionProps.userData);
            var basePosition = userData["hifiHomeTransformerKey"].basePosition;
            var baseRotation = userData["hifiHomeTransformerKey"].baseRotation;
            littleVersionProps.position = basePosition;
            littleVersionProps.rotation = baseRotation;
            // print('transformer  new version ' + JSON.stringify(littleVersionProps));
            var littleTransformer = Entities.addEntity(littleVersionProps);
            print('little transformer:: ' + littleTransformer);
            _this.removeSelf();
        },

        removeSelf: function() {
            print('transformer should remove itself')
            var success = Entities.deleteEntity(_this.entityID);
            print('transformer actually deleted self: ' + success);
        },
    };

    function getJointData(avatar) {
        //can you do this for an arbitrary model?
        var allJointData = [];
        var jointNames = MyAvatar.jointNames;
        jointNames.forEach(function(joint, index) {
            var translation = MyAvatar.getJointTranslation(index);
            var rotation = MyAvatar.getJointRotation(index)
            allJointData.push({
                joint: joint,
                index: index,
                translation: translation,
                rotation: rotation
            });
        });

        return allJointData;
    }

    function getAvatarFootOffset() {
        var data = getJointData();
        var upperLeg, lowerLeg, foot, toe, toeTop;
        data.forEach(function(d) {

            var jointName = d.joint;
            if (jointName === "RightUpLeg") {
                upperLeg = d.translation.y;
            }
            if (jointName === "RightLeg") {
                lowerLeg = d.translation.y;
            }
            if (jointName === "RightFoot") {
                foot = d.translation.y;
            }
            if (jointName === "RightToeBase") {
                toe = d.translation.y;
            }
            if (jointName === "RightToe_End") {
                toeTop = d.translation.y
            }
        })

        var myPosition = MyAvatar.position;
        var offset = upperLeg + lowerLeg + foot + toe + toeTop;
        offset = offset / 100;
        return offset
    }

    return new Transformer();
})