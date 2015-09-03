(function() {

    var popSound;
    this.preload = function(entityID) {
        //  console.log('bubble preload')
        this.entityID = entityID;
        popSound = SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop.wav");

    }

    this.collisionWithEntity = function(myID, otherID, collision) {

        Entities.deleteEntity(myID);
        this.burstBubbleSound(collision.contactPoint)

    };

    this.unload = function(entityID) {
        //  console.log('bubble unload')
        // this.properties = Entities.getEntityProperties(entityID);
        //var location = this.properties.position;
        //this.burstBubbleSound();
    };



    this.burstBubbleSound = function(location) {

        var audioOptions = {
            volume: 0.5,
            position: location
        }

        //Audio.playSound(popSound, audioOptions);

    }


})