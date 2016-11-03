var MAPPING_NAME = "com.highfidelity.rightClickExample";
var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from(Controller.Hardware.Keyboard.RightMouseClicked).to(function (value) {
    print("Keyboard.RightMouseClicked");
});
Controller.enableMapping(MAPPING_NAME);

Script.scriptEnding.connect(function () {
    Controller.disableMapping(MAPPING_NAME);
});