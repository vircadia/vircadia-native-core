(function() {
    Script.include('utils.js?' + Date.now());
    Script.include('spawnShortbow.js?' + Date.now());
    Script.include('shortbowGameManager.js?' + Date.now());

    this.entityID = null;
    var gameManager = null;
    this.preload = function(entityID) {
        this.entityID = entityID;

        var props = Entities.getEntityProperties(entityID, ['position', 'userData']);
        var data = utils.parseJSON(props.userData);
        if (data === undefined) {
            print("Error parsing shortbow entity userData, returning");
            return;
        }

        var rootPosition = props.position;

        var bowPositions = [];
        var spawnPositions = [];
        for (var i = 0; i < TEMPLATES.length; ++i) {
            var template = TEMPLATES[i];
            if (template.name === "SB.BowSpawn") {
                //bowPositions.push(Vec3.sum(rootPosition, template.localPosition));
                bowPositions.push(template.localPosition);
                //Vec3.print("Pushing bow position", Vec3.sum(rootPosition, template.localPosition));
            } else if (template.name === "SB.EnemySpawn") {
                //spawnPositions.push(Vec3.sum(rootPosition, template.localPosition));
                spawnPositions.push(template.localPosition);
                //Vec3.print("Pushing spawnposition", template.localPosition));
            }
        }

        gameManager = new ShortbowGameManager(this.entityID, bowPositions, spawnPositions);
    };
    this.unload = function() {
        if (gameManager) {
            gameManager.cleanup();
            gameManager = null;
        }
    };
});
