//  skyboxTest.js
//  examples/tests/skybox
//
//  Created by Zach Pomerantz on  3/10/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  This test cycles through different variations on the skybox with a mouseclick.
//  For the test to pass, you should observe the following cycle:
//  - Procedural skybox (no texture, no color)
//  - Procedural skybox (no texture, with color)
//  - Procedural skybox (with texture, no color)
//  - Procedural skybox (with texture, with color)
//  - Color skybox (no texture)
//  - Color skybox (with texture)
//  - Texture skybox (no color)
//
//  As you run the test, descriptions of the expected rendered skybox will appear as overlays.
//
//  NOTE: This does not test uniforms/textures applied to a procedural shader through userData.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var PX_URL = Script.resolvePath('px.fs');
var PX_RGBA_URL = Script.resolvePath('px_rgba.fs');
var PX_TEX_URL = Script.resolvePath('px_tex.fs');
var PX_TEX_RGBA_URL = Script.resolvePath('px_tex_rgba.fs');

var TEX_URL = 'https://hifi-public.s3.amazonaws.com/alan/Playa/Skies/Test-Sky_out.png';
var NO_TEX = '';

var COLOR = { red: 255, green: 0, blue: 255 };
var NO_COLOR = { red: 0, green: 0, blue: 0 };

var data = { ProceduralEntity: { shaderUrl: PX_URL } };

var zone = Entities.addEntity({
    type: 'Zone',
    dimensions: { x: 50, y: 50, z: 50 },
    position: MyAvatar.position,
    backgroundMode: 'skybox'
});
var text = Overlays.addOverlay('text', {
    text: 'Click this box to advance tests; note that red value cycling means white->light blue',
    x: Window.innerWidth / 2 - 250, y: Window.innerHeight / 2 - 25,
    width: 500, height: 50
});

print('Zone:', zone);
print('Text:', text);

var edits = [
    ['Red value should cycle', getEdit(PX_URL, NO_TEX, NO_COLOR)],
    ['Red value should cycle, no green', getEdit(PX_RGBA_URL, NO_TEX, COLOR)],
    ['Red value should cycle, each face tinted differently', getEdit(PX_TEX_URL, TEX_URL, NO_COLOR)],
    ['Red value should cycle, each face tinted differently, no green', getEdit(PX_TEX_RGBA_URL, TEX_URL, COLOR)],
    ['No green', getEdit(null, NO_TEX, COLOR)],
    ['Each face colored differently, no green', getEdit(null, TEX_URL, COLOR)],
    ['Each face colored differently', getEdit(null, TEX_URL, NO_COLOR)],
];

Controller.mousePressEvent.connect(function(e) { if (Overlays.getOverlayAtPoint(e) === text) next(); });

Script.scriptEnding.connect(function() {
    Overlays.deleteOverlay(text);
    Entities.deleteEntity(zone);
});

var i = 0;
function next() {
    var edit = edits[i];
    Overlays.editOverlay(text, { text: edit[0] });
    Entities.editEntity(zone, edit[1]);
    i++;
    i %= edits.length;
}

function getEdit(px, url, color) {
    return { userData: px ? getUserData(px) : '', backgroundMode: 'skybox', skybox: { url: url, color: color } }
}
function getUserData(px) { return JSON.stringify({ ProceduralEntity: { shaderUrl: px } }); }

