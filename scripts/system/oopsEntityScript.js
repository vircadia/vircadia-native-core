(function() {
    var PROTOCOL_MISMATCH = 1;
    var NOT_AUTHORIZED = 3;

    this.entityID = Uuid.NULL;
    this.preload = function(entityID) {
        this.entityID = entityID;
    };
    function pingError() {
        if(this.entityID === undefined) {
            var entities = Entities.findEntitiesByName("Oops Dialog", MyAvatar.position, 10);
            if(entities.length > 0) {
                this.entityID = entities[0];
            }
        }
        var error = location.lastDomainConnectionError;
        var newModel = "";
        var hostedSite = Script.resourcesPath() + "meshes/redirect";
        if (error === PROTOCOL_MISMATCH) {
            newModel = "oopsDialog_protocol";
        } else if (error === NOT_AUTHORIZED) {
            newModel = "oopsDialog_auth";
        } else {
            newModel = "oopsDialog_vague";
        }
        var props = Entities.getEntityProperties(this.entityID, ["modelURL"]);
        var newModelURL = hostedSite + newModel + ".fbx";
        if(props.modelURL !== newModelURL) {
            var newFileURL = newModelURL + "/" + newModel + ".png";
            var newTextures = {"file16": newFileURL, "file17": newFileURL};
            Entities.editEntity(this.entityID, {modelURL: newModelURL, originalTextures: JSON.stringify(newTextures)});
        }
    };
    var ping = Script.setInterval(pingError, 5000);

    function cleanup() {
        Script.clearInterval(ping);
    };

    Script.scriptEnding.connect(cleanup);
});
