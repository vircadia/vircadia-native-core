var fireScriptURL = Script.resolvePath("managers/fireManager.js");
Script.include(fireScriptURL);

var fireManager = new FireManager();

fireManager.reset();



function cleanup() {
    fireManager.reset();
}

Script.scriptEnding.connect(cleanup);
