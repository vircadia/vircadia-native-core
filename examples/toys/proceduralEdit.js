Script.include("../toys/magBalls/constants.js");

OmniToolModuleType = "ProceduralEdit"

OmniToolModules.ProceduralEdit = function(omniTool, entityId) {
    this.omniTool = omniTool;
    this.entityId = entityId;
    
    var window = new WebWindow('Procedural', "file:///C:/Users/bdavis/Git/hifi/examples/toys/proceduralEdit/ui.html", 640, 480, false);
    window.setVisible(true);
    
}
