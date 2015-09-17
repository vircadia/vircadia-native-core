var fireScriptURL = Script.resolvePath("managers/fireManager.js");
Script.include(fireScriptURL);

var blockScriptURL = Script.resolvePath("managers/blockManager.js");
Script.include(blockScriptURL);

var fireManager = new FireManager({
    x: 551.5435791015625,
    y: 494.87728881835938,
    z: 502.01531982421875
});

fireManager.reset();

var blockManager = new BlockManager({
                    x: 548.3,
                    y: 495.55,
                    z: 504.4
                });
blockManager.reset();



function cleanup() {
    fireManager.reset();
    blockManager.reset();
}

Script.scriptEnding.connect(cleanup);