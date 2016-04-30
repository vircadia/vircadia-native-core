//
//  lightLoader.js
//
//  Created by James Pollack @imgntn on 12/15/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Loads a test scene showing sliders that you can grab and move to change entity properties.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var grabScript = Script.resolvePath('../controllers/handControllerGrab.js?' + Math.random(0 - 100));
Script.load(grabScript);
var lightModifier = Script.resolvePath('lightModifier.js?' + Math.random(0 - 100));
Script.load(lightModifier);
Script.setTimeout(function() {
    var lightModifierTestScene = Script.resolvePath('lightModifierTestScene.js?' + Math.random(0 - 100));
    Script.load(lightModifierTestScene);
}, 750)