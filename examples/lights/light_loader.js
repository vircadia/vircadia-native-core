var grabScript = Script.resolvePath('../controllers/handControllerGrab.js');
Script.load(grabScript);
var lightModifier = Script.resolvePath('light_modifier.js');
Script.load(lightModifier);
Script.setTimeout(function() {
    var lightModifierTestScene = Script.resolvePath('light_modifier_test_scene.js');
    Script.load(lightModifierTestScene);
}, 750)