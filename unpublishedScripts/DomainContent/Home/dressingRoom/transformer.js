// when you drop a doll and it hits the dressing room platform it transforms into a big version. 
// the small doll is destroyed and a new small doll is put on the shelf


(function() {
    var TRIGGER_DISTANCE = 0.25;
    var TRANSFORMATION_SOUND_URL = '';

    function Transformer() {
        return this;
    }

    Transformer.prototype = {
        rotatorBlock: null,
        transformationSound: null,
        preload: function(entityID) {
            this.entityID = entityID;
            this.initialProperties = Entities.getEntityProperties(entityID);
            this.transformationSound = SoundCache.getSound(TRANSFORMATION_SOUND_URL);
        },

        collisionWithEntity: function(myID, otherID, collisionInfo) {
            var otherProps = Entities.getEntityProperties(otherID);
            if (otherProps.name = "hifi-home-dressing-room-transformer-collider") {
                // this.playTransformationSound(collisionInfo.contactPoint);
                // this.createTransformationParticles();
                this.findRotatorBlock();
            }
        },

        playTransformationSound: function(position) {
            Audio.playSound(this.transformationSound, {
                position: position,
                volume: 0.5
            });
        },

        createTransformationParticles: function() {
            var particleProps = {};
            Entities.addEntity(particleProps);
        },

        findRotatorBlock: function() {
            var myProps = Entities.getEntityProperties(this.entityID);
            var results = Entities.findEntities(myProps.position, 10);
            results.forEach(function(result) {
                var resultProps = Entities.getEntityProperties(result);
                if (resultProps.name === "hifi-home-dressing-room-rotator-block") {
                    this.rotatorBlock = result;
                }
            });

        },

        removeCurrentBigVersion: function() {
            var myProps = Entities.getEntityProperties(this.entityID);
            var results = Entities.findEntities(myProps.position, 10);
            results.forEach(function(result) {
                var resultProps = Entities.getEntityProperties(result);
                if (resultProps.name === "hifi-home-dressing-room-big-transformer") {
                    Entities.deleteEntity(result)
                }
            });

            this.createBigVersion(myProps);
        },

        createBigVersion: function(smallProps) {
            var rotatorProps = Entities.getEntityProperties(this.rotatorBlock);
            var bigVersionProps = {
                name: "hifi-home-dressing-room-big-transformer",
                type: 'Model',
                parentID: this.rotatorBlock,
                modelURL: smallProps.modelURL,
                position: this.putTransformerOnRotatorBlock(),
                rotation: rotatorProps.rotation,
                userData: JSON.stringify({
                    'grabbableKey': {
                        'grabbable:', false
                    },
                    'hifiHomeKey': {
                        'reset': true
                    }
                }),
            }

            Entities.addEntity(bigVersionProps);
            this.putNewVersionOnShelf();
        },

        putTransformerOnRotatorBlock: function() {

        },

        putNewVersionOnShelf: function() {
            var littleVersionProps = Entities.getEntityProperties(this.entityID);
            var userData = JSON.parse(littleVersionProps.userData);
            var basePosition = userData["hifiHomeTransformerKey"].basePosition;
            littleVersionProps.position = basePosition;
            Entities.addEntity(littleVersionProps);
            this.removeSelf();
        },

        removeSelf: function() {
            Entities.deleteEntity(this.entityID);
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