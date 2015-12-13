(function() {

  function Box () {
        this.oldColor = {};
        this.oldColorKnown = false;
    }

    Box.prototype = {
        preload: function(entityID) {
            print("preload");

            this.entityID = entityID;
            this.storeOldColor(entityID);
        },
        startDistantGrab:function(){

        },
        continueDistantGrab:function(){

        },
        releaseGrab:function(){

        }
        setHand:function(){
            
        }

    };

    return new Box();
});