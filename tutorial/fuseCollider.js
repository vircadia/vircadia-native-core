(function() {
    Script.include('utils.js');

    var Fuse = function() {
    };
    Fuse.prototype = {
        onLit: function() {
            print("LIT", this.entityID);
            var fuseID = Utils.findEntity({ name: "tutorial/equip/fuse" }, 20); 
            Entities.callEntityMethod(fuseID, "light");
        },
        preload: function(entityID) {
            this.entityID = entityID;
        },
    };
    return new Fuse();
});
