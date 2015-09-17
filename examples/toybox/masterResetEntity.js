var fireScriptURL = Script.resolvePath("managers/fireManager.js");
Script.include(fireScriptURL);

var blockScriptURL = Script.resolvePath("managers/blockManager.js");
Script.include(blockScriptURL);

var fireManager = new FireManager();
fireManager.reset();

var blockManager = new BlockManager();
blockManager.reset();



function cleanup() {
    fireManager.reset();
    blockManager.reset();
}

Script.scriptEnding.connect(cleanup);
