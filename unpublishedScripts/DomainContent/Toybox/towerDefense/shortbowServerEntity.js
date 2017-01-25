(function() {
    Script.include('utils.js?' + Date.now());
    Script.include('playWaveGame.js?' + Date.now());
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

        // Generate goal that the enemies try to get to
        goalPosition = Vec3.sum(rootPosition, { x: 0, y: -10, z: -20 });
        const BASES_HEIGHT = 16;
        const ROOF_HEIGHT = 0.2;

        goalPosition.y += BASES_HEIGHT - ROOF_HEIGHT;

        var platformID = data.platformID;
        var buttonID = data.buttonID;
        var waveDisplayID = data.waveDisplayID;
        var livesDisplayID = data.livesDisplayID;
        var scoreDisplayID = data.scoreDisplayID;
        var highScoreDisplayID = data.highScoreDisplayID;

        const BASES_SIZE = 15;
        var goalPositionFront = Vec3.sum(goalPosition, { x: 0, y: 0, z: BASES_SIZE / 2 });

        var bowPositions = [];
        var spawnPositions = [];
        for (var i = 0; i < TEMPLATES.length; ++i) {
            var template = TEMPLATES[i];
            if (template.name === "SB.BowSpawn") {
                bowPositions.push(Vec3.sum(rootPosition, template.localPosition));
                Vec3.print("Pushing bow position", Vec3.sum(rootPosition, template.localPosition));
            } else if (template.name === "SB.EnemySpawn") {
                spawnPositions.push(Vec3.sum(rootPosition, template.localPosition));
                Vec3.print("Pushing spawnposition", Vec3.sum(rootPosition, template.localPosition));
            }
        }

        gameManager = new ShortbowGameManager(rootPosition, goalPositionFront, bowPositions, spawnPositions, this.entityID, buttonID, waveDisplayID, scoreDisplayID, livesDisplayID, highScoreDisplayID);
    };
    this.unload = function() {
        if (gameManager) {
            gameManager.cleanup();
            gameManager = null;
        }
    };
});
