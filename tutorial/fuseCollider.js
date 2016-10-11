(function() {
    Script.include('utils.js');

    var Fuse = function() {
    };
    Fuse.prototype = {
        onLit: function() {
            print("fuseCollider.js | Lit", this.entityID);
            //var fuseID = Utils.findEntity({ name: "tutorial/equip/fuse" }, 20); 
            var fuseID = "{c8944a13-9acb-4d77-b1ee-851845e98357}"
            Entities.callEntityMethod(fuseID, "light");
        },
        preload: function(entityID) {
            this.entityID = entityID;
        },
    };
    return new Fuse();
});
