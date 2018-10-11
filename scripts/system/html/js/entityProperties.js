//  entityProperties.js
//
//  Created by Ryan Huffman on 13 Nov 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global alert, augmentSpinButtons, clearTimeout, console, document, Element, 
   EventBridge, JSONEditor, openEventBridge, setTimeout, window, _ $ */
    
const ICON_FOR_TYPE = {
    Box: "V",
    Sphere: "n",
    Shape: "n",
    ParticleEffect: "&#xe004;",
    Model: "&#xe008;",
    Web: "q",
    Image: "&#xe02a;",
    Text: "l",
    Light: "p",
    Zone: "o",
    PolyVox: "&#xe005;",
    Multiple: "&#xe000;",
    PolyLine: "&#xe01b;",
    Material: "&#xe00b;"
};  

const PI = 3.14159265358979;
const DEGREES_TO_RADIANS = PI / 180.0;
const RADIANS_TO_DEGREES = 180.0 / PI;

const NO_SELECTION = "<i>No selection</i>";

const GROUPS = [
    {
        id: "base",
        properties: [
            {
                label: NO_SELECTION,
                type: "icon",
                icons: ICON_FOR_TYPE,
                propertyID: "type",
            },
            {
                label: "Name",
                type: "string",
                propertyID: "name",
            },
            {
                label: "ID",
                type: "string",
                propertyID: "id",
                readOnly: true,
            },
            {
                label: "Parent",
                type: "string",
                propertyID: "parentID",
            },
            {
                label: "Locked",
                glyph: "&#xe006;",
                type: "bool",
                propertyID: "locked",
            },
            {
                label: "Visible",
                glyph: "&#xe007;",
                type: "bool",
                propertyID: "visible",
            },
        ]
    },
    {
        id: "shape",
        addToGroup: "base",
        properties: [
            {
                label: "Shape",
                type: "dropdown",
                options: { Cube: "Box", Sphere: "Sphere", Tetrahedron: "Tetrahedron", Octahedron: "Octahedron", 
                           Icosahedron: "Icosahedron", Dodecahedron: "Dodecahedron", Hexagon: "Hexagon", 
                           Triangle: "Triangle", Octagon: "Octagon", Cylinder: "Cylinder", Cone: "Cone", 
                           Circle: "Circle", Quad: "Quad" },
                propertyID: "shape",
            },
            {
                label: "Color",
                type: "color",
                propertyID: "color",
            },
        ]
    },
    {
        id: "text",
        addToGroup: "base",
        properties: [
            {
                label: "Text",
                type: "string",
                propertyID: "text",
            },
            {
                label: "Text Color",
                type: "color",
                propertyID: "textColor",
            },
            {
                label: "Background Color",
                type: "color",
                propertyID: "backgroundColor",
            },
            {
                label: "Line Height",
                type: "number",
                min: 0,
                step: 0.005,
                fixedDecimals: 4,
                unit: "m",
                propertyID: "lineHeight"
            },
            {
                label: "Face Camera",
                type: "bool",
                propertyID: "faceCamera"
            },
        ]
    },
    {
        id: "zone",
        addToGroup: "base",
        properties: [
            {
                label: "Flying Allowed",
                type: "bool",
                propertyID: "flyingAllowed",
            },
            {
                label: "Ghosting Allowed",
                type: "bool",
                propertyID: "ghostingAllowed",
            },
            {
                label: "Filter",
                type: "string",
                propertyID: "filterURL",
            },
            {
                label: "Key Light",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyID: "keyLightMode",
                
            },
            {
                label: "Key Light Color",
                type: "color",
                propertyID: "keyLight.color",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Light Intensity",
                type: "number",
                min: 0,
                max: 10,
                step: 0.1,
                fixedDecimals: 2,
                propertyID: "keyLight.intensity",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Light Altitude",
                type: "number",
                fixedDecimals: 2,
                unit: "deg",
                propertyID: "keyLight.direction.y",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Light Azimuth",
                type: "number",
                fixedDecimals: 2,
                unit: "deg",
                propertyID: "keyLight.direction.x",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Cast Shadows",
                type: "bool",
                propertyID: "keyLight.castShadows",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Skybox",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyID: "skyboxMode",
            },
            {
                label: "Skybox Color",
                type: "color",
                propertyID: "skybox.color",
                showPropertyRule: { "skyboxMode": "enabled" },
            },
            {
                label: "Skybox URL",
                type: "string",
                propertyID: "skybox.url",
                showPropertyRule: { "skyboxMode": "enabled" },
            },
            {
                type: "buttons",
                buttons: [ { id: "copy", label: "Copy URL To Ambient", 
                             className: "black", onClick: copySkyboxURLToAmbientURL } ],
                propertyID: "copyURLToAmbient",
                showPropertyRule: { "skyboxMode": "enabled" },
            },
            {
                label: "Ambient Light",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyID: "ambientLightMode",
            },
            {
                label: "Ambient Intensity",
                type: "number",
                min: 0,
                max: 10,
                step: 0.1,
                fixedDecimals: 2,
                propertyID: "ambientLight.ambientIntensity",
                showPropertyRule: { "ambientLightMode": "enabled" },
            },
            {
                label: "Ambient URL",
                type: "string",
                propertyID: "ambientLight.ambientURL",
                showPropertyRule: { "ambientLightMode": "enabled" },
            },
            {
                label: "Haze",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyID: "hazeMode",
            },
            {
                label: "Range",
                type: "number",
                min: 5,
                max: 10000,
                step: 5,
                fixedDecimals: 0,
                unit: "m",
                propertyID: "haze.hazeRange",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Use Altitude",
                type: "bool",
                propertyID: "haze.hazeAltitudeEffect",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Base",
                type: "number",
                min: -1000,
                max: 1000,
                step: 10,
                fixedDecimals: 0,
                unit: "m",
                propertyID: "haze.hazeBaseRef",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Ceiling",
                type: "number",
                min: -1000,
                max: 5000,
                step: 10,
                fixedDecimals: 0,
                unit: "m",
                propertyID: "haze.hazeCeiling",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Haze Color",
                type: "color",
                propertyID: "haze.hazeColor",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Background Blend",
                type: "number",
                min: 0.0,
                max: 1.0,
                step: 0.01,
                fixedDecimals: 2,
                propertyID: "haze.hazeBackgroundBlend",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Enable Glare",
                type: "bool",
                propertyID: "haze.hazeEnableGlare",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Glare Color",
                type: "color",
                propertyID: "haze.hazeGlareColor",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Glare Angle",
                type: "number",
                min: 0,
                max: 180,
                step: 1,
                fixedDecimals: 0,
                propertyID: "haze.hazeGlareAngle",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Bloom",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyID: "bloomMode",
            },
            {
                label: "Bloom Intensity",
                type: "number",
                min: 0,
                max: 1,
                step: 0.01,
                fixedDecimals: 2,
                propertyID: "bloom.bloomIntensity",
                showPropertyRule: { "bloomMode": "enabled" },
            },
            {
                label: "Bloom Threshold",
                type: "number",
                min: 0,
                min: 1,
                step: 0.01,
                fixedDecimals: 2,
                propertyID: "bloom.bloomThreshold",
                showPropertyRule: { "bloomMode": "enabled" },
            },
            {
                label: "Bloom Size",
                type: "number",
                min: 0,
                min: 2,
                step: 0.01,
                fixedDecimals: 2,
                propertyID: "bloom.bloomSize",
                showPropertyRule: { "bloomMode": "enabled" },
            },
        ]
    },
    {
        id: "model",
        addToGroup: "base",
        properties: [
            {
                label: "Model",
                type: "string",
                propertyID: "modelURL",
            },
            {
                label: "Collision Shape",
                type: "dropdown",
                options: { "none": "No Collision", "box": "Box", "sphere": "Sphere", "compound": "Compound" , 
                           "simple-hull": "Basic - Whole model", "simple-compound": "Good - Sub-meshes" , 
                           "static-mesh": "Exact - All polygons (non-dynamic only)" },
                propertyID: "shapeType",
            },
            {
                label: "Compound Shape",
                type: "string",
                propertyID: "compoundShapeURL",
            },
            {
                label: "Animation",
                type: "string",
                propertyID: "animation.url",
            },
            {
                label: "Play Automatically",
                type: "bool",
                propertyID: "animation.running",
            },
            {
                label: "Allow Transition",
                type: "bool",
                propertyID: "animation.allowTranslation",
            },
            {
                label: "Loop",
                type: "bool",
                propertyID: "animation.loop",
            },
            {
                label: "Hold",
                type: "bool",
                propertyID: "animation.hold",
            },
            {
                label: "Animation Frame",
                type: "number",
                propertyID: "animation.currentFrame",
            },
            {
                label: "First Frame",
                type: "number",
                propertyID: "animation.firstFrame",
            },
            {
                label: "Last Frame",
                type: "number",
                propertyID: "animation.lastFrame",
            },
            {
                label: "Animation FPS",
                type: "number",
                propertyID: "animation.fps",
            },
            {
                label: "Texture",
                type: "textarea",
                propertyID: "textures",
            },
            {
                label: "Original Texture",
                type: "textarea",
                propertyID: "originalTextures",
                readOnly: true,
            },
        ]
    },
    {
        id: "image",
        addToGroup: "base",
        properties: [
            {
                label: "Image",
                type: "string",
                propertyID: "image",
            },
        ]
    },
    {
        id: "web",
        addToGroup: "base",
        properties: [
            {
                label: "Source",
                type: "string",
                propertyID: "sourceUrl",
            },
            {
                label: "Source Resolution",
                type: "number",
                propertyID: "dpi",
            },
        ]
    },
    {
        id: "light",
        addToGroup: "base",
        addToGroup: "base",
        properties: [
            {
                label: "Light Color",
                type: "color",
                propertyID: "lightColor",
                propertyName: "color", // actual entity property name
            },
            {
                label: "Intensity",
                type: "number",
                min: 0,
                step: 0.1,
                fixedDecimals: 1,
                propertyID: "intensity",
            },
            {
                label: "Fall-Off Radius",
                type: "number",
                min: 0,
                step: 0.1,
                fixedDecimals: 1,
                unit: "m",
                propertyID: "falloffRadius",
            },
            {
                label: "Spotlight",
                type: "bool",
                propertyID: "isSpotlight",
            },
            {
                label: "Spotlight Exponent",
                type: "number",
                step: 0.01,
                fixedDecimals: 2,
                propertyID: "exponent",
            },
            {
                label: "Spotlight Cut-Off",
                type: "number",
                step: 0.01,
                fixedDecimals: 2,
                propertyID: "cutoff",
            },
        ]
    },
    {
        id: "material",
        addToGroup: "base",
        properties: [
            {
                label: "Material URL",
                type: "string",
                propertyID: "materialURL",
            },
            {
                label: "Material Data",
                type: "textarea",
                buttons: [ { id: "clear", label: "Clear Material Data", className: "red", onClick: clearMaterialData }, 
                           { id: "edit", label: "Edit as JSON", className: "blue", onClick: newJSONMaterialEditor },
                           { id: "save", label: "Save Material Data", className: "black", onClick: saveMaterialData } ],
                propertyID: "materialData",
            },
            {
                label: "Submesh to Replace",
                type: "number",
                min: 0,
                step: 1,
                propertyID: "submeshToReplace",
            },
            {
                label: "Material Name to Replace",
                type: "string",
                propertyID: "materialNameToReplace",
            },
            {
                label: "Select Submesh",
                type: "bool",
                propertyID: "selectSubmesh",
            },
            {
                label: "Priority",
                type: "number",
                min: 0,
                propertyID: "priority",
            },
            {
                label: "Material Position",
                type: "vec2",
                min: 0,
                min: 1,
                step: 0.1,
                vec2Type: "xy",
                subLabels: [ "x", "y" ],
                propertyID: "materialMappingPos",
            },
            {
                label: "Material Scale",
                type: "vec2",
                min: 0,
                step: 0.1,
                vec2Type: "wh",
                subLabels: [ "width", "height" ],
                propertyID: "materialMappingScale",
            },
            {
                label: "Material Rotation",
                type: "number",
                step: 0.1,
                fixedDecimals: 2,
                unit: "deg",
                propertyID: "materialMappingRot",
            },
        ]
    },
    {
        id: "spatial",
        label: "SPATIAL",
        properties: [
            {
                label: "Position",
                type: "vec3",
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "m",
                propertyID: "position",
            },
            {
                label: "Rotation",
                type: "vec3",
                step: 0.1,
                vec3Type: "pyr",
                subLabels: [ "pitch", "yaw", "roll" ],
                unit: "deg",
                propertyID: "rotation",
            },
            {
                label: "Dimension",
                type: "vec3",
                step: 0.1,
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "m",
                propertyID: "dimensions",
            },
            {
                label: "Scale",
                type: "number",
                defaultValue: 100,
                unit: "%",
                buttons: [ { id: "rescale", label: "Rescale", className: "blue", onClick: rescaleDimensions }, 
                           { id: "reset", label: "Reset Dimensions", className: "red", onClick: resetToNaturalDimensions } ],
                propertyID: "scale",
            },
            {
                label: "Pivot",
                type: "vec3",
                step: 0.1,
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "(ratio of dimension)",
                propertyID: "registrationPoint",
            },
            {
                label: "Align",
                type: "buttons",
                buttons: [ { id: "selection", label: "Selection to Grid", className: "black", onClick: moveSelectionToGrid },
                           { id: "all", label: "All to Grid", className: "black", onClick: moveAllToGrid } ],
                propertyID: "alignToGrid",
            },
        ]
    },
    {
        id: "collision",
        label: "COLLISION",
        twoColumn: true,
        properties: [
            {
                label: "Collides",
                type: "bool",
                inverse: true,
                propertyID: "collisionless",
                column: -1, // before two columns div
            },
            {
                label: "Dynamic",
                type: "bool",
                propertyID: "dynamic",
                column: -1, // before two columns div
            },
            {
                label: "Collides With",
                type: "sub-header",
                propertyID: "collidesWithHeader", // not actually a property but used for naming/storing this element
                showPropertyRule: { "collisionless": "false" },
                column: 1,
            },
            {
                label: "",
                type: "sub-header",
                propertyID: "collidesWithHeaderHelper", // not actually a property but used for naming/storing this element
                showPropertyRule: { "collisionless": "false" },
                column: 2,
            },
            {
                label: "Static Entities",
                type: "bool",
                propertyID: "static",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 1,
            },
            {
                label: "Dynamic Entities",
                type: "bool",
                propertyID: "dynamic",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 2,
            },
            {
                label: "Kinematic Entities",
                type: "bool",
                propertyID: "kinematic",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 1,
            },
            {
                label: "My Avatar",
                type: "bool",
                propertyID: "myAvatar",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 2,
            },
            {
                label: "Other Avatars",
                type: "bool",
                propertyID: "otherAvatar",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 1,
            },
            {
                label: "Collision sound URL",
                type: "string",
                propertyID: "collisionSoundURL",
                showPropertyRule: { "collisionless": "false" },
                // having no column number means place this after two columns div
            },
        ]
    },
    {
        id: "behavior",
        label: "BEHAVIOR",
        twoColumn: true,
        properties: [
            {
                label: "Grabbable",
                type: "bool",
                propertyID: "grabbable",
                column: 1,
            },
            {
                label: "Triggerable",
                type: "bool",
                propertyID: "triggerable",
                column: 2,
            },
            {
                label: "Cloneable",
                type: "bool",
                propertyID: "cloneable",
                column: 1,
            },
            {
                label: "Ignore inverse kinematics",
                type: "bool",
                propertyID: "ignoreIK",
                column: 2,
            },
            {
                label: "Clone Lifetime",
                type: "number",
                unit: "s",
                propertyID: "cloneLifetime",
                showPropertyRule: { "cloneable": "true" },
                column: 1,
            },
            {
                label: "Clone Limit",
                type: "number",
                propertyID: "cloneLimit",
                showPropertyRule: { "cloneable": "true" },
                column: 1,
            },
            {
                label: "Clone Dynamic",
                type: "bool",
                propertyID: "cloneDynamic",
                showPropertyRule: { "cloneable": "true" },
                column: 1,
            },
            {
                label: "Clone Avatar Entity",
                type: "bool",
                propertyID: "cloneAvatarEntity",
                showPropertyRule: { "cloneable": "true" },
                column: 1,
            },
            {   // below properties having no column number means place them after two columns div
                label: "Can cast shadow",
                type: "bool",
                propertyID: "canCastShadow",
            },
            {
                label: "Script",
                type: "string",
                buttons: [ { id: "reload", label: "F", className: "glyph", onClick: reloadScripts } ],
                propertyID: "script",
            },
            {
                label: "Server Script",
                type: "string",
                buttons: [ { id: "reload", label: "F", className: "glyph", onClick: reloadServerScripts } ],
                propertyID: "serverScripts",
            },
            {
                label: "Lifetime",
                type: "number",
                unit: "s",
                propertyID: "lifetime",
            },
            {
                label: "User Data",
                type: "textarea",
                buttons: [ { id: "clear", label: "Clear User Data", className: "red", onClick: clearUserData }, 
                           { id: "edit", label: "Edit as JSON", className: "blue", onClick: newJSONEditor },
                           { id: "save", label: "Save User Data", className: "black", onClick: saveUserData } ],
                propertyID: "userData",
            },
        ]
    },
    {
        id: "physics",
        label: "PHYSICS",
        properties: [
            {
                label: "Linear Velocity",
                type: "vec3",
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "m/s",
                propertyID: "velocity",
            },
            {
                label: "Linear Damping",
                type: "number",
                fixedDecimals: 2,
                propertyID: "damping",
            },
            {
                label: "Angular Velocity",
                type: "vec3",
                multiplier: DEGREES_TO_RADIANS,
                vec3Type: "pyr",
                subLabels: [ "pitch", "yaw", "roll" ],
                unit: "deg/s",
                propertyID: "angularVelocity",
            },
            {
                label: "Angular Damping",
                type: "number",
                fixedDecimals: 4,
                propertyID: "angularDamping",
            },
            {
                label: "Bounciness",
                type: "number",
                fixedDecimals: 4,
                propertyID: "restitution",
            },
            {
                label: "Friction",
                type: "number",
                fixedDecimals: 4,
                propertyID: "friction",
            },
            {
                label: "Density",
                type: "number",
                fixedDecimals: 4,
                propertyID: "density",
            },
            {
                label: "Gravity",
                type: "vec3",
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "m/s<sup>2</sup>",
                propertyID: "gravity",
            },
            {
                label: "Acceleration",
                type: "vec3",
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "m/s<sup>2</sup>",
                propertyID: "acceleration",
            },
        ]
    },
];

const GROUPS_PER_TYPE = {
  None: [ 'base', 'spatial', 'collision', 'behavior', 'physics' ],
  Shape: [ 'base', 'shape', 'spatial', 'collision', 'behavior', 'physics' ],
  Text: [ 'base', 'text', 'spatial', 'collision', 'behavior', 'physics' ],
  Zone: [ 'base', 'zone', 'spatial', 'collision', 'behavior', 'physics' ],
  Model: [ 'base', 'model', 'spatial', 'collision', 'behavior', 'physics' ],
  Image: [ 'base', 'image', 'spatial', 'collision', 'behavior', 'physics' ],
  Web: [ 'base', 'web', 'spatial', 'collision', 'behavior', 'physics' ],
  Light: [ 'base', 'light', 'spatial', 'collision', 'behavior', 'physics' ],
  Material: [ 'base', 'material', 'spatial', 'behavior' ],
  ParticleEffect: [ 'base', 'spatial', 'behavior', 'physics' ],
  Multiple: [ 'base', 'spatial', 'collision', 'behavior', 'physics' ],
};

const EDITOR_TIMEOUT_DURATION = 1500;
const KEY_P = 80; // Key code for letter p used for Parenting hotkey.

const MATERIAL_PREFIX_STRING = "mat::";

const PENDING_SCRIPT_STATUS = "[ Fetching status ]";

var elGroups = {};
var properties = {};
var colorPickers = {};
var selectedEntityProperties;
var lastEntityID = null;

function debugPrint(message) {
    EventBridge.emitWebEvent(
        JSON.stringify({
            type: "print",
            message: message
        })
    );
}


// GENERAL PROPERTY/GROUP FUNCTIONS

function getPropertyElement(propertyID) {
    return properties[propertyID].el;
}

function enableChildren(el, selector) {
    var elSelectors = el.querySelectorAll(selector);
    for (var selectorIndex = 0; selectorIndex < elSelectors.length; ++selectorIndex) {
        elSelectors[selectorIndex].removeAttribute('disabled');
    }
}

function disableChildren(el, selector) {
    var elSelectors = el.querySelectorAll(selector);
    for (var selectorIndex = 0; selectorIndex < elSelectors.length; ++selectorIndex) {
        elSelectors[selectorIndex].setAttribute('disabled', 'disabled');
    }
}

function enableProperties() {
    enableChildren(document.getElementById("properties-list"), "input, textarea, checkbox, .dropdown dl, .color-picker");
    enableChildren(document, ".colpick");
    var elLocked = getPropertyElement("locked");

    if (elLocked.checked === false) {
        removeStaticUserData();
        removeStaticMaterialData();
    }
}

function disableProperties() {
    disableChildren(document.getElementById("properties-list"), "input, textarea, checkbox, .dropdown dl, .color-picker");
    disableChildren(document, ".colpick");
    for (var pickKey in colorPickers) {
        colorPickers[pickKey].colpickHide();
    }
    var elLocked = getPropertyElement("locked");

    if (elLocked.checked === true) {
        if ($('#property-userData-editor').css('display') === "block") {
            showStaticUserData();
        }
        if ($('#property-materialData-editor').css('display') === "block") {
            showStaticMaterialData();
        }
    }
}

function showPropertyElement(propertyID, show) {
    let elProperty = properties[propertyID].el;
    let elNode = elProperty;
    if (elNode.nodeName !== "DIV") {
        let elParent = elProperty.parentNode;
        if (elParent === undefined && elProperty instanceof Array) {
            elParent = elProperty[0].parentNode;
        }
        if (elParent !== undefined) {
            elNode = elParent;
        }
    }
    elNode.style.display = show ? "table" : "none";
}

function resetProperties() {
    for (let propertyID in properties) { 
        let elProperty = properties[propertyID].el;      
        let propertyData = properties[propertyID].data;
        
        switch (propertyData.type) {
            case 'string': {
                elProperty.value = "";
                break;
            }
            case 'bool': {
                elProperty.checked = false;
                break;
            }
            case 'number': {
                if (propertyData.defaultValue !== undefined) {
                    elProperty.value = propertyData.defaultValue;
                } else { 
                    elProperty.value = "";
                }
                break;
            }
            case 'vec3': 
            case 'vec2': {
                // vec2/vec3 are array of 2/3 elInput numbers
                elProperty[0].value = "";
                elProperty[1].value = "";
                if (elProperty[2] !== undefined) {
                    elProperty[2].value = "";
                }
                break;
            }
            case 'color': {
                // color is array of color picker and 3 elInput numbers
                elProperty[0].style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                elProperty[1].value = "";
                elProperty[2].value = "";
                elProperty[3].value = "";
                break;
            }
            case 'dropdown': {
                elProperty.value = "";
                setDropdownText(elProperty);
                break;
            }
            case 'textarea': {
                elProperty.value = "";
                setTextareaScrolling(elProperty);
                break;
            }
            case 'icon': {
                // icon is array of elSpan (icon glyph) and elLabel
                elProperty[0].style.display = "none";
                elProperty[1].innerHTML = propertyData.label;
                break;
            }
        }
        
        let showPropertyRules = properties[propertyID].showPropertyRules;
        if (showPropertyRules !== undefined) {
            for (let propertyToHide in showPropertyRules) {
                showPropertyElement(propertyToHide, false);
            }
        }
    }
}

function showGroupsForType(type) {
    if (type === "Box" || type === "Sphere") {
        type = "Shape";
    }
    
    let typeGroups = GROUPS_PER_TYPE[type];

    for (let groupKey in elGroups) {
        let elGroup = elGroups[groupKey];
        if (typeGroups && typeGroups.indexOf(groupKey) > -1) {
            elGroup.style.display = "block";
        } else {
            elGroup.style.display = "none";
        }
    }
}


// PROPERTY UPDATE FUNCTIONS

function updateProperty(propertyName, propertyValue) {
    let propertyUpdate = {};
    // if this is a compound property name (i.e. animation.running) then split it by . up to 3 times
    let splitPropertyName = propertyName.split('.');
    if (splitPropertyName.length > 1) {
        let propertyGroupName = splitPropertyName[0];
        let subPropertyName = splitPropertyName[1];
        propertyUpdate[propertyGroupName] = {};
        if (splitPropertyName.length === 3) { 
            let subSubPropertyName = splitPropertyName[2];
            propertyUpdate[propertyGroupName][subPropertyName] = {};
            propertyUpdate[propertyGroupName][subPropertyName][subSubPropertyName] = propertyValue;
        } else {
            propertyUpdate[propertyGroupName][subPropertyName] = propertyValue;
        }
    } else {
        propertyUpdate[propertyName] = propertyValue;
    }
    updateProperties(propertyUpdate);
}

function updateProperties(propertiesToUpdate) {
    EventBridge.emitWebEvent(JSON.stringify({
        id: lastEntityID,
        type: "update",
        properties: propertiesToUpdate
    }));
}


function createEmitTextPropertyUpdateFunction(propertyName) {
    return function() {
        updateProperty(propertyName, this.value);
    };
}

function createEmitCheckedPropertyUpdateFunction(propertyName, inverse) {
    return function() {
        updateProperty(propertyName, inverse ? !this.checked : this.checked);
    };
}

function createEmitNumberPropertyUpdateFunction(propertyName, decimals) {
    decimals = ((decimals === undefined) ? 4 : decimals);
    return function() {
        let value = parseFloat(this.value).toFixed(decimals);
        updateProperty(propertyName, value);
    };
}

function createEmitVec2PropertyUpdateFunction(propertyName, elX, elY) {
    return function () {
        let newValue = {
            x: elX.value,
            y: elY.value
        };
        updateProperty(propertyName, newValue);
    };
}

function createEmitVec2PropertyUpdateFunctionWithMultiplier(propertyName, elX, elY, multiplier) {
    return function () {
        let newValue = {
            x: elX.value * multiplier,
            y: elY.value * multiplier
        };
        updateProperty(propertyName, newValue);
    };
}

function createEmitVec3PropertyUpdateFunction(propertyName, elX, elY, elZ) {
    return function() {
        let newValue = {
            x: elX.value,
            y: elY.value,
            z: elZ ? elZ.value : 0
        };
        updateProperty(propertyName, newValue);
    };
}

function createEmitVec3PropertyUpdateFunctionWithMultiplier(propertyName, elX, elY, elZ, multiplier) {
    return function() {
        let newValue = {
            x: elX.value * multiplier,
            y: elY.value * multiplier,
            z: elZ.value * multiplier
        };
        updateProperty(propertyName, newValue);
    };
}

function createEmitColorPropertyUpdateFunction(propertyName, elRed, elGreen, elBlue) {
    return function() {
        emitColorPropertyUpdate(propertyName, elRed.value, elGreen.value, elBlue.value);
    };
}

function emitColorPropertyUpdate(propertyName, red, green, blue) {
    let newValue = {
        red: red,
        green: green,
        blue: blue
    };
    updateProperty(propertyName, newValue);
}

function updateCheckedSubProperty(propertyName, propertyValue, subPropertyElement, subPropertyString) {
    if (subPropertyElement.checked) {
        if (propertyValue.indexOf(subPropertyString)) {
            propertyValue += subPropertyString + ',';
        }
    } else {
        // We've unchecked, so remove
        propertyValue = propertyValue.replace(subPropertyString + ",", "");
    }
    updateProperty(propertyName, propertyValue);
}

function createImageURLUpdateFunction(propertyName) {
    return function () {
        var newTextures = JSON.stringify({ "tex.picture": this.value });
        updateProperty(propertyName, newTextures);
    };
}


// PROPERTY ELEMENT CREATION FUNCTIONS

function createStringProperty(property, elProperty, elLabel) {    
    let propertyName = property.name;
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.setAttribute("class", "property text");
    
    let elInput = document.createElement('input');
    elInput.setAttribute("id", elementID);
    elInput.setAttribute("type", "text"); 
    if (propertyData.readOnly) {
        elInput.readOnly = true;
    }
    
    elInput.addEventListener('change', createEmitTextPropertyUpdateFunction(propertyName));
    
    elProperty.appendChild(elLabel);
    elProperty.appendChild(elInput);
    
    if (propertyData.buttons !== undefined) {
        addButtons(elProperty, elementID, propertyData.buttons, false);
    }
    
    return elInput;
}

function createBoolProperty(property, elProperty, elLabel) {   
    let propertyName = property.name;
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.setAttribute("class", "property checkbox");
                        
    if (propertyData.glyph !== undefined) {
        elLabel.innerText = " " + propertyData.label;
        let elSpan = document.createElement('span');
        elSpan.innerHTML = propertyData.glyph;
        elLabel.insertBefore(elSpan, elLabel.firstChild);
    }
    
    let elInput = document.createElement('input');
    elInput.setAttribute("id", elementID);
    elInput.setAttribute("type", "checkbox");
    
    elProperty.appendChild(elInput);
    elProperty.appendChild(elLabel);
    
    let subPropertyOf = propertyData.subPropertyOf;
    if (subPropertyOf !== undefined) {
        elInput.addEventListener('change', function() {
            updateCheckedSubProperty(subPropertyOf, selectedEntityProperties[subPropertyOf], elInput, propertyName);
        });
    } else {
        elInput.addEventListener('change', createEmitCheckedPropertyUpdateFunction(propertyName, propertyData.inverse));
    }
    
    return elInput;
}

function createVec3Property(property, elProperty, elLabel) {
    let propertyName = property.name;
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.setAttribute("class", "property " + propertyData.vec3Type + " fstuple");
    
    let elTuple = document.createElement('div');
    elTuple.setAttribute("class", "tuple");
    
    addUnit(propertyData.unit, elLabel);
    
    elProperty.appendChild(elLabel);
    elProperty.appendChild(elTuple);
    
    let elInputX = createTupleNumberInput(elTuple, elementID, propertyData.subLabels[0], 
                                          propertyData.min, propertyData.max, propertyData.step);
    let elInputY = createTupleNumberInput(elTuple, elementID, propertyData.subLabels[1], 
                                          propertyData.min, propertyData.max, propertyData.step);
    let elInputZ = createTupleNumberInput(elTuple, elementID, propertyData.subLabels[2], 
                                          propertyData.min, propertyData.max, propertyData.step);
    
    let inputChangeFunction;
    if (propertyData.multiplier !==  undefined) { 
        inputChangeFunction = createEmitVec3PropertyUpdateFunctionWithMultiplier(propertyName, elInputX, elInputY, 
                                                                                 elInputZ, propertyData.multiplier);
    } else {
        inputChangeFunction = createEmitVec3PropertyUpdateFunction(propertyName, elInputX, elInputY, elInputZ);
    }
    elInputX.addEventListener('change', inputChangeFunction);
    elInputY.addEventListener('change', inputChangeFunction);
    elInputZ.addEventListener('change', inputChangeFunction);
    
    return [ elInputX, elInputY, elInputZ ];
}

function createVec2Property(property, elProperty, elLabel) {  
    let propertyName = property.name;
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.setAttribute("class", "property " + propertyData.vec2Type + " fstuple");
                        
    let elTuple = document.createElement('div');
    elTuple.setAttribute("class", "tuple");
    
    addUnit(propertyData.unit, elLabel);
    
    elProperty.appendChild(elLabel);
    elProperty.appendChild(elTuple);
    
    let elInputX = createTupleNumberInput(elTuple, elementID, propertyData.subLabels[0], 
                                          propertyData.min, propertyData.max, propertyData.step);
    let elInputY = createTupleNumberInput(elTuple, elementID, propertyData.subLabels[1], 
                                          propertyData.min, propertyData.max, propertyData.step);
    
    let inputChangeFunction;    
    if (propertyData.multiplier !==  undefined) { 
        inputChangeFunction = createEmitVec2PropertyUpdateFunctionWithMultiplier(propertyName, elInputX, elInputY, 
                                                                                 propertyData.multiplier);
    } else {
        inputChangeFunction = createEmitVec2PropertyUpdateFunctionWithMultiplier(propertyName, elInputX, elInputY);
    }
    elInputX.addEventListener('change', inputChangeFunction);
    elInputY.addEventListener('change', inputChangeFunction);
    
    return [elInputX, elInputY];
}

function createColorProperty(property, elProperty, elLabel) {
    let propertyName = property.name;
    let elementID = property.elementID;
    
    elProperty.setAttribute("class", "property rgb fstuple");
    
    let elColorPicker = document.createElement('div');
    elColorPicker.setAttribute("class", "color-picker");
    elColorPicker.setAttribute("id", elementID);
    
    let elTuple = document.createElement('div');
    elTuple.setAttribute("class", "tuple");
    
    elProperty.appendChild(elColorPicker);
    elProperty.appendChild(elLabel);
    elProperty.appendChild(elTuple);
    
    let elInputR = createTupleNumberInput(elTuple, elementID, "red", 0, 255, 1);
    let elInputG = createTupleNumberInput(elTuple, elementID, "green", 0, 255, 1);
    let elInputB = createTupleNumberInput(elTuple, elementID, "blue", 0, 255, 1);
    
    let inputChangeFunction = createEmitColorPropertyUpdateFunction(propertyName, elInputR, elInputG, elInputB);  
    elInputR.addEventListener('change', inputChangeFunction);
    elInputG.addEventListener('change', inputChangeFunction);
    elInputB.addEventListener('change', inputChangeFunction);
    
    let colorPickerID = "#" + elementID;
    colorPickers[colorPickerID] = $(colorPickerID).colpick({
        colorScheme: 'dark',
        layout: 'hex',
        color: '000000',
        submit: false, // We don't want to have a submission button
        onShow: function(colpick) {
            $(colorPickerID).attr('active', 'true');
            // The original color preview within the picker needs to be updated on show because
            // prior to the picker being shown we don't have access to the selections' starting color.
            colorPickers[colorPickerID].colpickSetColor({
                "r": elInputR.value,
                "g": elInputG.value,
                "b": elInputB.value
            });
        },
        onHide: function(colpick) {
            $(colorPickerID).attr('active', 'false');
        },
        onChange: function(hsb, hex, rgb, el) {
            $(el).css('background-color', '#' + hex);
            emitColorPropertyUpdate(propertyName, rgb.r, rgb.g, rgb.b);
        }
    });
    
    return [elColorPicker, elInputR, elInputG, elInputB];
}

function createDropdownProperty(property, propertyID, elProperty, elLabel) { 
    let propertyName = property.name;
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.setAttribute("class", "property dropdown");
                        
    let elInput = document.createElement('select');
    elInput.setAttribute("id", elementID);
    elInput.setAttribute("propertyID", propertyID);
    
    for (let optionKey in propertyData.options) {
        let option = document.createElement('option');
        option.value = optionKey;
        option.text = propertyData.options[optionKey];
        elInput.add(option);
    }
    
    elInput.addEventListener('change', createEmitTextPropertyUpdateFunction(propertyName));
    
    elProperty.appendChild(elLabel);
    elProperty.appendChild(elInput);
    
    return elInput;
}

function createNumberProperty(property, elProperty, elLabel) { 
    let propertyName = property.name;
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.setAttribute("class", "property number");
                        
    addUnit(propertyData.unit, elLabel);
    
    let elInput = document.createElement('input');
    elInput.setAttribute("id", elementID);
    elInput.setAttribute("type", "number");
    
    let defaultValue = propertyData.defaultValue;
    if (defaultValue !== undefined) {
        elInput.value = defaultValue;
    }
    
    let fixedDecimals = propertyData.fixedDecimals !== undefined ? propertyData.fixedDecimals : 0;
    elInput.addEventListener('change', createEmitNumberPropertyUpdateFunction(propertyName, fixedDecimals));
    
    elProperty.appendChild(elLabel);
    elProperty.appendChild(elInput);
    
    if (propertyData.buttons !== undefined) {
        addButtons(elProperty, elementID, propertyData.buttons, true);
    }
    
    return elInput;
}

function createTextareaProperty(property, elProperty, elLabel) {   
    let propertyName = property.name;
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.setAttribute("class", "property textarea");
                        
    elProperty.appendChild(elLabel);
    
    if (propertyData.buttons !== undefined) {
        addButtons(elProperty, elementID, propertyData.buttons, true);
    }
    
    let elInput = document.createElement('textarea');
    elInput.setAttribute("id", elementID);
    if (propertyData.readOnly) {
        elInput.readOnly = true;
    }                   
    
    elInput.addEventListener('change', createEmitTextPropertyUpdateFunction(propertyName));
    
    elProperty.appendChild(elInput);
    
    return elInput;
}

function createIconProperty(property, elProperty, elLabel) { 
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.setAttribute("class", "property value");
    
    elLabel.setAttribute("id", elementID);
    elLabel.innerHTML = " " + propertyData.label;
    
    let elSpan = document.createElement('span');
    elSpan.setAttribute("id", elementID + "-icon");

    elProperty.appendChild(elSpan);
    elProperty.appendChild(elLabel);
    
    return [ elSpan, elLabel ];
}

function createButtonsProperty(property, elProperty, elLabel) {
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.setAttribute("class", "property text");
                        
    let hasLabel = propertyData.label !== undefined;
    if (hasLabel) {
        elProperty.appendChild(elLabel);
    }
    
    if (propertyData.buttons !== undefined) {
        addButtons(elProperty, elementID, propertyData.buttons, hasLabel);
    }
    
    return elProperty;
}

function createTupleNumberInput(elTuple, propertyElementID, subLabel, min, max, step) {
    let elementID = propertyElementID + "-" + subLabel.toLowerCase();
    
    let elDiv = document.createElement('div');
    let elLabel = document.createElement('label');
    elLabel.innerText = subLabel[0].toUpperCase() + subLabel.slice(1) + ":";
    elLabel.setAttribute("for", elementID);
    
    let elInput = document.createElement('input');
    elInput.setAttribute("id", elementID);
    elInput.setAttribute("type", "number");
    elInput.setAttribute("class", subLabel);
    
    elDiv.appendChild(elInput);
    elDiv.appendChild(elLabel);
    elTuple.appendChild(elDiv);
    
    return elInput;
}

function addUnit(unit, elLabel) {
    if (unit !== undefined) {
        let elSpan = document.createElement('span');
        elSpan.setAttribute("class", "unit");
        elSpan.innerHTML = unit;
        elLabel.appendChild(elSpan);
    }
}

function addButtons(elProperty, propertyID, buttons, newRow) {
    let elDiv = document.createElement('div');
    elDiv.setAttribute("class", "row");
    
    buttons.forEach(function(button) {
        let elButton = document.createElement('input');
        elButton.setAttribute("type", "button");
        elButton.setAttribute("class", button.className);
        elButton.setAttribute("id", propertyID + "-button-" + button.id);
        elButton.setAttribute("value", button.label);
        elButton.addEventListener("click", button.onClick);
        if (newRow) {
            elDiv.appendChild(elButton);
        } else {
            elProperty.appendChild(elButton);
        }
    });
    
    if (newRow) {
        elProperty.appendChild(document.createElement('br'));
        elProperty.appendChild(elDiv);
    }
}


// BUTTON CALLBACKS

function rescaleDimensions() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "action",
        action: "rescaleDimensions",
        percentage: parseFloat(document.getElementById("property-scale").value)
    }));
}

function moveSelectionToGrid() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "action",
        action: "moveSelectionToGrid"
    }));
}

function moveAllToGrid() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "action",
        action: "moveAllToGrid"
    }));
}

function resetToNaturalDimensions() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "action",
        action: "resetToNaturalDimensions"
    }));
}

function reloadScripts() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: "action",
        action: "reloadClientScripts"
    }));
}

function reloadServerScripts() {
    // invalidate the current status (so that same-same updates can still be observed visually)
    document.getElementById("property-serverScripts-status").innerText = PENDING_SCRIPT_STATUS;
        EventBridge.emitWebEvent(JSON.stringify({
        type: "action",
        action: "reloadServerScripts"
    }));
}

function copySkyboxURLToAmbientURL() {
    let skyboxURL = getPropertyElement("skybox.url").value;
    getPropertyElement("ambientLight.ambientURL").value = skyboxURL;
    updateProperty("ambientLight.ambientURL", skyboxURL);
}


// USER DATA FUNCTIONS

function clearUserData() {
    let elUserData = getPropertyElement("userData");
    deleteJSONEditor();
    elUserData.value = "";
    showUserDataTextArea();
    showNewJSONEditorButton();
    hideSaveUserDataButton();
    updateProperty('userData', elUserData.value);
}

function newJSONEditor() {
    deleteJSONEditor();
    createJSONEditor();
    var data = {};
    setEditorJSON(data);
    hideUserDataTextArea();
    hideNewJSONEditorButton();
    showSaveUserDataButton();
}

function saveUserData() {
    saveJSONUserData(true);
}

function setUserDataFromEditor(noUpdate) {
    var json = null;
    try {
        json = editor.get();
    } catch (e) {
        alert('Invalid JSON code - look for red X in your code ', +e);
    }
    if (json === null) {
        return;
    } else {
        var text = editor.getText();
        if (noUpdate === true) {
            EventBridge.emitWebEvent(
                JSON.stringify({
                    id: lastEntityID,
                    type: "saveUserData",
                    properties: {
                        userData: text
                    }
                })
            );
            return;
        } else {
            updateProperty('userData', text);
        }
    }
}

function multiDataUpdater(groupName, updateKeyPair, userDataElement, defaults, removeKeys) {
    var propertyUpdate = {};
    var parsedData = {};
    var keysToBeRemoved = removeKeys ? removeKeys : [];
    try {
        if ($('#property-userData-editor').css('height') !== "0px") {
            // if there is an expanded, we want to use its json.
            parsedData = getEditorJSON();
        } else {
            parsedData = JSON.parse(userDataElement.value);
        }
    } catch (e) {
        // TODO: Should an alert go here?
    }

    if (!(groupName in parsedData)) {
        parsedData[groupName] = {};
    }
    var keys = Object.keys(updateKeyPair);
    keys.forEach(function (key) {
        if (updateKeyPair[key] !== null && updateKeyPair[key] !== "null") {
            if (updateKeyPair[key] instanceof Element) {
                if (updateKeyPair[key].type === "checkbox") {
                    parsedData[groupName][key] = updateKeyPair[key].checked;
                } else {
                    var val = isNaN(updateKeyPair[key].value) ? updateKeyPair[key].value : parseInt(updateKeyPair[key].value);
                    parsedData[groupName][key] = val;
                }
            } else {
                parsedData[groupName][key] = updateKeyPair[key];
            }
        } else if (defaults[key] !== null && defaults[key] !== "null") {
            parsedData[groupName][key] = defaults[key];
        }
    });
    keysToBeRemoved.forEach(function(key) {
        if (parsedData[groupName].hasOwnProperty(key)) {
            delete parsedData[groupName][key];
        }
    });
    
    if (Object.keys(parsedData[groupName]).length === 0) {
        delete parsedData[groupName];
    }
    if (Object.keys(parsedData).length > 0) {
        propertyUpdate.userData = JSON.stringify(parsedData);
    } else {
        propertyUpdate.userData = '';
    }

    userDataElement.value = propertyUpdate.userData;

    updateProperties(propertyUpdate);
}
function userDataChanger(groupName, keyName, values, userDataElement, defaultValue, removeKeys) {
    var val = {}, def = {};
    val[keyName] = values;
    def[keyName] = defaultValue;
    multiDataUpdater(groupName, val, userDataElement, def, removeKeys);
}

var editor = null;

function createJSONEditor() {
    var container = document.getElementById("property-userData-editor");
    var options = {
        search: false,
        mode: 'tree',
        modes: ['code', 'tree'],
        name: 'userData',
        onModeChange: function() {
            $('.jsoneditor-poweredBy').remove();
        },
        onError: function(e) {
            alert('JSON editor:' + e);
        },
        onChange: function() {
            var currentJSONString = editor.getText();

            if (currentJSONString === '{"":""}') {
                return;
            }
            $('#property-userData-button-save').attr('disabled', false);


        }
    };
    editor = new JSONEditor(container, options);
}

function showSaveUserDataButton() {
    $('#property-userData-button-save').show();
}

function hideSaveUserDataButton() {
    $('#property-userData-button-save').hide();
}

function disableSaveUserDataButton() {
    $('#property-userData-button-save').attr('disabled', true);
}

function showNewJSONEditorButton() {
    $('#property-userData-button-edit').show();
}

function hideNewJSONEditorButton() {
    $('#property-userData-button-edit').hide();
}

function showUserDataTextArea() {
    $('#property-userData').show();
}

function hideUserDataTextArea() {
    $('#property-userData').hide();
}

function hideUserDataSaved() {
    $('#property-userData-saved').hide();
}

function showStaticUserData() {
    if (editor !== null) {
        $('#property-userData-static').show();
        $('#property-userData-static').css('height', $('#property-userData-editor').height());
        $('#property-userData-static').text(editor.getText());
    }
}

function removeStaticUserData() {
    $('#property-userData-static').hide();
}

function setEditorJSON(json) {
    editor.set(json);
    if (editor.hasOwnProperty('expandAll')) {
        editor.expandAll();
    }
}

function getEditorJSON() {
    return editor.get();
}

function deleteJSONEditor() {
    if (editor !== null) {
        editor.destroy();
        editor = null;
    }
}

var savedJSONTimer = null;

function saveJSONUserData(noUpdate) {
    setUserDataFromEditor(noUpdate);
    $('#property-userData-saved').show();
    $('#property-userData-button-save').attr('disabled', true);
    if (savedJSONTimer !== null) {
        clearTimeout(savedJSONTimer);
    }
    savedJSONTimer = setTimeout(function() {
        hideUserDataSaved();
    }, EDITOR_TIMEOUT_DURATION);
}


// MATERIAL DATA FUNCTIONS

function clearMaterialData() {
    let elMaterialData = getPropertyElement("materialData");
    deleteJSONMaterialEditor();
    elMaterialData.value = "";
    showMaterialDataTextArea();
    showNewJSONMaterialEditorButton();
    hideSaveMaterialDataButton();
    updateProperty('materialData', elMaterialData.value);
}

function newJSONMaterialEditor() {
    deleteJSONMaterialEditor();
    createJSONMaterialEditor();
    var data = {};
    setMaterialEditorJSON(data);
    hideMaterialDataTextArea();
    hideNewJSONMaterialEditorButton();
    showSaveMaterialDataButton();
}

function saveMaterialData() {
    saveJSONMaterialData(true);
}

function setMaterialDataFromEditor(noUpdate) {
    var json = null;
    try {
        json = materialEditor.get();
    } catch (e) {
        alert('Invalid JSON code - look for red X in your code ', +e);
    }
    if (json === null) {
        return;
    } else {
        var text = materialEditor.getText();
        if (noUpdate === true) {
            EventBridge.emitWebEvent(
                JSON.stringify({
                    id: lastEntityID,
                    type: "saveMaterialData",
                    properties: {
                        materialData: text
                    }
                })
            );
            return;
        } else {
            updateProperty('materialData', text);
        }
    }
}

var materialEditor = null;

function createJSONMaterialEditor() {
    var container = document.getElementById("property-materialData-editor");
    var options = {
        search: false,
        mode: 'tree',
        modes: ['code', 'tree'],
        name: 'materialData',
        onModeChange: function() {
            $('.jsoneditor-poweredBy').remove();
        },
        onError: function(e) {
            alert('JSON editor:' + e);
        },
        onChange: function() {
            var currentJSONString = materialEditor.getText();

            if (currentJSONString === '{"":""}') {
                return;
            }
            $('#property-materialData-button-save').attr('disabled', false);


        }
    };
    materialEditor = new JSONEditor(container, options);
}

function showSaveMaterialDataButton() {
    $('#property-materialData-button-save').show();
}

function hideSaveMaterialDataButton() {
    $('#property-materialData-button-save').hide();
}

function disableSaveMaterialDataButton() {
    $('#property-materialData-button-save').attr('disabled', true);
}

function showNewJSONMaterialEditorButton() {
    $('#property-materialData-button-edit').show();
}

function hideNewJSONMaterialEditorButton() {
    $('#property-materialData-button-edit').hide();
}

function showMaterialDataTextArea() {
    $('#property-materialData').show();
}

function hideMaterialDataTextArea() {
    $('#property-materialData').hide();
}

function hideMaterialDataSaved() {
    $('#property-materialData-saved').hide();
}

function showStaticMaterialData() {
    if (materialEditor !== null) {
        $('#property-materialData-static').show();
        $('#property-materialData-static').css('height', $('#property-materialData-editor').height());
        $('#property-materialData-static').text(materialEditor.getText());
    }
}

function removeStaticMaterialData() {
    $('#property-materialData-static').hide();
}

function setMaterialEditorJSON(json) {
    materialEditor.set(json);
    if (materialEditor.hasOwnProperty('expandAll')) {
        materialEditor.expandAll();
    }
}

function getMaterialEditorJSON() {
    return materialEditor.get();
}

function deleteJSONMaterialEditor() {
    if (materialEditor !== null) {
        materialEditor.destroy();
        materialEditor = null;
    }
}

var savedMaterialJSONTimer = null;

function saveJSONMaterialData(noUpdate) {
    setMaterialDataFromEditor(noUpdate);
    $('#property-materialData-saved').show();
    $('#property-materialData-button-save').attr('disabled', true);
    if (savedMaterialJSONTimer !== null) {
        clearTimeout(savedMaterialJSONTimer);
    }
    savedMaterialJSONTimer = setTimeout(function() {
        hideMaterialDataSaved();
    }, EDITOR_TIMEOUT_DURATION);
}

function bindAllNonJSONEditorElements() {
    var inputs = $('input');
    var i;
    for (i = 0; i < inputs.length; i++) {
        var input = inputs[i];
        var field = $(input);
        // TODO FIXME: (JSHint) Functions declared within loops referencing 
        //             an outer scoped variable may lead to confusing semantics.
        field.on('focus', function(e) {
            if (e.target.id === "property-userData-button-edit" || e.target.id === "property-userData-button-clear" || 
                e.target.id === "property-materialData-button-edit" || e.target.id === "property-materialData-button-clear") {
                return;
            } else {
                if ($('#property-userData-editor').css('height') !== "0px") {
                    saveUserData();
                }
                if ($('#property-materialData-editor').css('height') !== "0px") {
                    saveMaterialData();
                }
            }
        });
    }
}


// DROPDOWN FUNCTIONS

function setDropdownText(dropdown) {
    let lis = dropdown.parentNode.getElementsByTagName("li");
    let text = "";
    for (let i = 0; i < lis.length; i++) {
        if (String(lis[i].getAttribute("value")) === String(dropdown.value)) {
            text = lis[i].textContent;
        }
    }
    dropdown.firstChild.textContent = text;
}

function toggleDropdown(event) {
    let element = event.target;
    if (element.nodeName !== "DT") {
        element = element.parentNode;
    }
    element = element.parentNode;
    let isDropped = element.getAttribute("dropped");
    element.setAttribute("dropped", isDropped !== "true" ? "true" : "false");
}

function setDropdownValue(event) {
    let dt = event.target.parentNode.parentNode.previousSibling;
    dt.value = event.target.getAttribute("value");
    dt.firstChild.textContent = event.target.textContent;

    dt.parentNode.setAttribute("dropped", "false");

    let evt = document.createEvent("HTMLEvents");
    evt.initEvent("change", true, true);
    dt.dispatchEvent(evt);
}


// TEXTAREA / PARENT MATERIAL NAME FUNCTIONS

function setTextareaScrolling(element) {
    var isScrolling = element.scrollHeight > element.offsetHeight;
    element.setAttribute("scrolling", isScrolling ? "true" : "false");
}

function showParentMaterialNameBox(number, elNumber, elString) {
    if (number) {
        $('#property-submeshToReplace').parent().show();
        $('#property-materialNameToReplace').parent().hide();
        elString.value = "";
    } else {
        $('#property-materialNameToReplace').parent().show();
        $('#property-submeshToReplace').parent().hide();
        elNumber.value = 0;
    }
}



function loaded() {
    openEventBridge(function() {    
        let elPropertiesList = document.getElementById("properties-list");
        
        GROUPS.forEach(function(group) {            
            let elGroup;
            if (group.addToGroup !== undefined) {
                let fieldset = document.getElementById("properties-" + group.addToGroup);
                elGroup = document.createElement('div');
                fieldset.appendChild(elGroup);
            } else {
                elGroup = document.createElement('fieldset');
                elGroup.setAttribute("class", "major");
                elGroup.setAttribute("id", "properties-" + group.id);
                elPropertiesList.appendChild(elGroup);
            }       

            if (group.label !== undefined) {
                let elLegend = document.createElement('legend');
                elLegend.innerText = group.label;
                elLegend.setAttribute("class", "section-header");
                let elSpan = document.createElement('span');
                elSpan.setAttribute("class", ".collapse-icon");
                elSpan.innerText = "M";
                elLegend.appendChild(elSpan);
                elGroup.appendChild(elLegend);
            }
                
            group.properties.forEach(function(propertyData) {
                let propertyType = propertyData.type;
                let propertyID = propertyData.propertyID;               
                let propertyName = propertyData.propertyName !== undefined ? propertyData.propertyName : propertyID;
                let propertyElementID = "property-" + propertyID;
                
                let elProperty;
                if (propertyType === "sub-header") {
                    elProperty = document.createElement('legend');
                    elProperty.innerText = propertyData.label;
                    elProperty.setAttribute("class", "sub-section-header");
                } else {
                    elProperty = document.createElement('div');
                    elProperty.setAttribute("id", "div-" + propertyElementID);
                }
                
                if (group.twoColumn && propertyData.column !== undefined && propertyData.column !== -1) {
                    let columnName = group.id + "column" + propertyData.column;
                    let elColumn = document.getElementById(columnName);
                    if (!elColumn) {
                        let columnDivName = group.id + "columnDiv";
                        let elColumnDiv = document.getElementById(columnDivName);
                        if (!elColumnDiv) {
                            elColumnDiv = document.createElement('div');
                            elColumnDiv.setAttribute("class", "two-column");
                            elColumnDiv.setAttribute("id", group.id + "columnDiv");
                            elGroup.appendChild(elColumnDiv);
                        }
                        elColumn = document.createElement('fieldset');
                        elColumn.setAttribute("class", "column");
                        elColumn.setAttribute("id", columnName);
                        elColumnDiv.appendChild(elColumn);
                    }
                    elColumn.appendChild(elProperty);
                } else {
                    elGroup.appendChild(elProperty);
                }
                
                let elLabel = document.createElement('label');
                elLabel.innerText = propertyData.label;
                elLabel.setAttribute("for", propertyElementID);
                
                properties[propertyID] = { data: propertyData, elementID: propertyElementID, name: propertyName };
                
                let property = properties[propertyID];
                
                switch (propertyType) {
                    case 'string': {
                        properties[propertyID].el = createStringProperty(property, elProperty, elLabel);
                        break;
                    }
                    case 'bool': {
                        properties[propertyID].el = createBoolProperty(property, elProperty, elLabel);
                        break;
                    }
                    case 'number': {
                        properties[propertyID].el = createNumberProperty(property, elProperty, elLabel);
                        break;
                    }
                    case 'vec3': {
                        properties[propertyID].el = createVec3Property(property, elProperty, elLabel);  
                        break;
                    }
                    case 'vec2': {
                        properties[propertyID].el = createVec2Property(property, elProperty, elLabel);  
                        break;
                    }
                    case 'color': {
                        properties[propertyID].el = createColorProperty(property, elProperty, elLabel);  
                        break;
                    }
                    case 'dropdown': {
                        properties[propertyID].el = createDropdownProperty(property, propertyID, elProperty, elLabel);
                        break;
                    }
                    case 'textarea': {
                        properties[propertyID].el = createTextareaProperty(property, elProperty, elLabel);
                        break;
                    }
                    case 'icon': {
                        properties[propertyID].el = createIconProperty(property, elProperty, elLabel);
                        break;
                    }
                    case 'buttons': {
                        properties[propertyID].el = createButtonsProperty(property, elProperty, elLabel);
                        break;
                    }
                    case 'sub-header': {
                        properties[propertyID].el = elProperty;
                        break;
                    }
                    default: {
                        console.log("EntityProperties - Unknown property type " + 
                                    propertyType + " set to property " + propertyID);
                        break;
                    }
                }
                
                let showPropertyRule = propertyData.showPropertyRule;
                if (showPropertyRule !== undefined) {
                    let dependentProperty = Object.keys(showPropertyRule)[0];
                    let dependentPropertyValue = showPropertyRule[dependentProperty];
                    if (properties[dependentProperty] === undefined) {
                        properties[dependentProperty] = {};
                    }
                    if (properties[dependentProperty].showPropertyRules === undefined) {
                        properties[dependentProperty].showPropertyRules = {};
                    }
                    properties[dependentProperty].showPropertyRules[propertyID] = dependentPropertyValue;
                }
            });
            
            elGroups[group.id] = elGroup;
        });
        
        if (window.EventBridge !== undefined) {
            EventBridge.scriptEventReceived.connect(function(data) {
                data = JSON.parse(data);
                if (data.type === "server_script_status") {
                    let elServerScriptError = document.getElementById("property-serverScripts-error");
                    let elServerScriptStatus = document.getElementById("property-serverScripts-status");
                    elServerScriptError.value = data.errorInfo;
                    // If we just set elServerScriptError's diplay to block or none, we still end up with
                    // it's parent contributing 21px bottom padding even when elServerScriptError is display:none.
                    // So set it's parent to block or none
                    elServerScriptError.parentElement.style.display = data.errorInfo ? "block" : "none";
                    if (data.statusRetrieved === false) {
                        elServerScriptStatus.innerText = "Failed to retrieve status";
                    } else if (data.isRunning) {
                        var ENTITY_SCRIPT_STATUS = {
                            pending: "Pending",
                            loading: "Loading",
                            error_loading_script: "Error loading script", // eslint-disable-line camelcase
                            error_running_script: "Error running script", // eslint-disable-line camelcase
                            running: "Running",
                            unloaded: "Unloaded"
                        };
                        elServerScriptStatus.innerText = ENTITY_SCRIPT_STATUS[data.status] || data.status;
                    } else {
                        elServerScriptStatus.innerText = "Not running";
                    }
                } else if (data.type === "update" && data.selections) {
                    if (data.selections.length === 0) {
                        if (lastEntityID !== null) {
                            if (editor !== null) {
                                saveUserData();
                                deleteJSONEditor();
                            }
                            if (materialEditor !== null) {
                                saveMaterialData();
                                deleteJSONMaterialEditor();
                            }
                        }
                        
                        resetProperties();
                        showGroupsForType("None");
                
                        deleteJSONEditor();
                        getPropertyElement("userData").value = "";
                        showUserDataTextArea();
                        showSaveUserDataButton();
                        showNewJSONEditorButton();
                        
                        deleteJSONMaterialEditor();
                        getPropertyElement("materialData").value = "";
                        showMaterialDataTextArea();
                        showSaveMaterialDataButton();
                        showNewJSONMaterialEditorButton();

                        disableProperties();
                    } else if (data.selections.length > 1) {
                        deleteJSONEditor();
                        deleteJSONMaterialEditor();
                        
                        let selections = data.selections;

                        let ids = [];
                        let types = {};
                        let numTypes = 0;

                        for (let i = 0; i < selections.length; i++) {
                            ids.push(selections[i].id);
                            let currentSelectedType = selections[i].properties.type;
                            if (types[currentSelectedType] === undefined) {
                                types[currentSelectedType] = 0;
                                numTypes += 1;
                            }
                            types[currentSelectedType]++;
                        }

                        let type = "Multiple";
                        if (numTypes === 1) {
                            type = selections[0].properties.type;
                        }
                        
                        resetProperties();
                        showGroupsForType(type);
                        
                        let typeProperty = properties["type"];
                        let elTypeProperty = typeProperty.el;
                        elTypeProperty[0].innerHTML = typeProperty.data.icons[type];
                        elTypeProperty[0].style.display = "inline-block";
                        elTypeProperty[1].innerHTML = type + " (" + data.selections.length + ")";
                        
                        disableProperties();
                    } else {
                        selectedEntityProperties = data.selections[0].properties;
                        
                        showGroupsForType(selectedEntityProperties.type);
                        
                        if (lastEntityID !== '"' + selectedEntityProperties.id + '"' && lastEntityID !== null) {
                            if (editor !== null) {
                               saveUserData();
                            }
                            if (materialEditor !== null) {
                                saveMaterialData();
                            }
                        }

                        let doSelectElement = lastEntityID === '"' + selectedEntityProperties.id + '"';

                        // the event bridge and json parsing handle our avatar id string differently.
                        lastEntityID = '"' + selectedEntityProperties.id + '"';

                        // HTML workaround since image is not yet a separate entity type
                        let IMAGE_MODEL_NAME = 'default-image-model.fbx';
                        if (selectedEntityProperties.type === "Model") {
                            let urlParts = selectedEntityProperties.modelURL.split('/');
                            let propsFilename = urlParts[urlParts.length - 1];

                            if (propsFilename === IMAGE_MODEL_NAME) {
                                selectedEntityProperties.type = "Image";
                            }
                        }
                        
                        for (let propertyID in properties) {
                            let property = properties[propertyID];
                            let elProperty = property.el;
                            let propertyData = property.data;
                            let propertyName = property.name;
                            
                            // if this is a compound property name (i.e. animation.running) 
                            // then split it by . up to 3 times to find property value
                            let propertyValue;
                            let splitPropertyName = propertyName.split('.');
                            if (splitPropertyName.length > 1) {
                                let propertyGroupName = splitPropertyName[0];
                                let subPropertyName = splitPropertyName[1];
                                let groupProperties = selectedEntityProperties[propertyGroupName];
                                if (groupProperties === undefined || groupProperties[subPropertyName] === undefined) {
                                    continue;
                                }
                                if (splitPropertyName.length === 3) { 
                                    let subSubPropertyName = splitPropertyName[2];
                                    propertyValue = groupProperties[subPropertyName][subSubPropertyName];
                                } else {
                                    propertyValue = groupProperties[subPropertyName];
                                }
                            } else {
                                propertyValue = selectedEntityProperties[propertyName];
                            }
                            
                            let isSubProperty = propertyData.subPropertyOf !== undefined;
                            if (elProperty === undefined || (propertyValue === undefined && !isSubProperty)) {
                                continue;
                            }
                            
                            switch (propertyData.type) {
                                case 'string': {
                                    elProperty.value = propertyValue;
                                    break;
                                }
                                case 'bool': {
                                    let inverse = propertyData.inverse !== undefined ? propertyData.inverse : false;
                                    if (isSubProperty) {
                                        let propertyValue = selectedEntityProperties[propertyData.subPropertyOf];
                                        let subProperties = propertyValue.split(",");
                                        let subPropertyValue = subProperties.indexOf(propertyID) > -1;
                                        elProperty.checked = inverse ? !subPropertyValue : subPropertyValue;
                                    } else {
                                        elProperty.checked = inverse ? !propertyValue : propertyValue;
                                    }
                                    break;
                                }
                                case 'number': {
                                    let fixedDecimals = propertyData.fixedDecimals !== undefined ? propertyData.fixedDecimals : 0;
                                    elProperty.value = propertyValue.toFixed(fixedDecimals);
                                    break;
                                }
                                case 'vec3':
                                case 'vec2': {
                                    // vec2/vec3 are array of 2/3 elInput numbers
                                    let multiplier = propertyData.multiplier !== undefined ? propertyData.multiplier : 1;
                                    elProperty[0].value = (propertyValue.x / multiplier).toFixed(4);
                                    elProperty[1].value = (propertyValue.y / multiplier).toFixed(4);
                                    if (elProperty[2] !== undefined) {
                                        elProperty[2].value = (propertyValue.z / multiplier).toFixed(4);
                                    }
                                    break;
                                }
                                case 'color': {
                                    // color is array of color picker and 3 elInput numbers
                                    elProperty[0].style.backgroundColor = "rgb(" + propertyValue.red + "," + 
                                                                          propertyValue.green + "," + 
                                                                          propertyValue.blue + ")";
                                    elProperty[1].value = propertyValue.red;
                                    elProperty[2].value = propertyValue.green;
                                    elProperty[3].value = propertyValue.blue;
                                    break;
                                }
                                case 'dropdown': {
                                    elProperty.value = propertyValue;
                                    setDropdownText(elProperty);
                                    break;
                                }
                                case 'textarea': {
                                    elProperty.value = propertyValue;
                                    setTextareaScrolling(elProperty);
                                    break;
                                }
                                case 'icon': {
                                    // icon is array of elSpan (icon glyph) and elLabel
                                    elProperty[0].innerHTML = propertyData.icons[propertyValue];
                                    elProperty[0].style.display = "inline-block";
                                    elProperty[1].innerHTML = propertyValue;
                                    break;
                                }
                            }
                            
                            let showPropertyRules = property.showPropertyRules;
                            if (showPropertyRules !== undefined) {
                                for (let propertyToShow in showPropertyRules) {
                                    let showIfThisPropertyValue = showPropertyRules[propertyToShow];
                                    let show = String(propertyValue) === String(showIfThisPropertyValue);
                                    showPropertyElement(propertyToShow, show);
                                }
                            }
                        }
                        
                        let elGrabbable = getPropertyElement("grabbable");
                        let elTriggerable = getPropertyElement("triggerable");
                        let elIgnoreIK = getPropertyElement("ignoreIK");
                        elGrabbable.checked = getPropertyElement("dynamic").checked;
                        elTriggerable.checked = false;
                        elIgnoreIK.checked = true;
                        let grabbablesSet = false;
                        let parsedUserData = {};
                        try {
                            parsedUserData = JSON.parse(selectedEntityProperties.userData);
                            if ("grabbableKey" in parsedUserData) {
                                grabbablesSet = true;
                                let grabbableData = parsedUserData.grabbableKey;
                                if ("grabbable" in grabbableData) {
                                    elGrabbable.checked = grabbableData.grabbable;
                                } else {
                                    elGrabbable.checked = true;
                                }
                                if ("triggerable" in grabbableData) {
                                    elTriggerable.checked = grabbableData.triggerable;
                                } else if ("wantsTrigger" in grabbableData) {
                                    elTriggerable.checked = grabbableData.wantsTrigger;
                                } else {
                                    elTriggerable.checked = false;
                                }
                                if ("ignoreIK" in grabbableData) {
                                    elIgnoreIK.checked = grabbableData.ignoreIK;
                                } else {
                                    elIgnoreIK.checked = true;
                                }
                            }
                        } catch (e) {
                            // TODO:  What should go here?
                        }
                        if (!grabbablesSet) {
                            elGrabbable.checked = true;
                            elTriggerable.checked = false;
                            elIgnoreIK.checked = true;
                        }
                        
                        if (selectedEntityProperties.type === "Image") {
                            let imageLink = JSON.parse(selectedEntityProperties.textures)["tex.picture"];
                            getPropertyElement("image").value = imageLink;
                        } else if (selectedEntityProperties.type === "Material") {
                            let elParentMaterialNameString = getPropertyElement("materialNameToReplace");
                            let elParentMaterialNameNumber = getPropertyElement("submeshToReplace");
                            let elParentMaterialNameCheckbox = getPropertyElement("selectSubmesh");
                            let parentMaterialName = selectedEntityProperties.parentMaterialName;
                            if (parentMaterialName.startsWith(MATERIAL_PREFIX_STRING)) {
                                elParentMaterialNameString.value = parentMaterialName.replace(MATERIAL_PREFIX_STRING, "");
                                showParentMaterialNameBox(false, elParentMaterialNameNumber, elParentMaterialNameString);
                                elParentMaterialNameCheckbox.checked = false;
                            } else {
                                elParentMaterialNameNumber.value = parseInt(parentMaterialName);
                                showParentMaterialNameBox(true, elParentMaterialNameNumber, elParentMaterialNameString);
                                elParentMaterialNameCheckbox.checked = true;
                            }
                        }
                        
                        let json = null;
                        try {
                            json = JSON.parse(selectedEntityProperties.userData);
                        } catch (e) {
                            // normal text
                            deleteJSONEditor();
                            getPropertyElement("userData").value = selectedEntityProperties.userData;
                            showUserDataTextArea();
                            showNewJSONEditorButton();
                            hideSaveUserDataButton();
                            hideUserDataSaved();
                        }
                        if (json !== null) {
                            if (editor === null) {
                                createJSONEditor();
                            }
                            setEditorJSON(json);
                            showSaveUserDataButton();
                            hideUserDataTextArea();
                            hideNewJSONEditorButton();
                            hideUserDataSaved();
                        }

                        let materialJson = null;
                        try {
                            materialJson = JSON.parse(selectedEntityProperties.materialData);
                        } catch (e) {
                            // normal text
                            deleteJSONMaterialEditor();
                            getPropertyElement("materialData").value = selectedEntityProperties.materialData;
                            showMaterialDataTextArea();
                            showNewJSONMaterialEditorButton();
                            hideSaveMaterialDataButton();
                            hideMaterialDataSaved();
                        }
                        if (materialJson !== null) {
                            if (materialEditor === null) {
                                createJSONMaterialEditor();
                            }
                            setMaterialEditorJSON(materialJson);
                            showSaveMaterialDataButton();
                            hideMaterialDataTextArea();
                            hideNewJSONMaterialEditorButton();
                            hideMaterialDataSaved();
                        }
                        
                        if (selectedEntityProperties.locked) {
                            disableProperties();
                            getPropertyElement("locked").removeAttribute('disabled');
                        } else {
                            enableProperties();
                            disableSaveUserDataButton();
                            disableSaveMaterialDataButton()
                        }
                        
                        let activeElement = document.activeElement;
                        if (doSelectElement && typeof activeElement.select !== "undefined") {
                            activeElement.select();
                        }
                    }
                }
            });
        }
        
        // Server Script Status
        let serverScriptProperty = properties["serverScripts"];
        let elServerScript = serverScriptProperty.el;
        let serverScriptElementID = serverScriptProperty.elementID;
        let elDiv = document.createElement('div');
        elDiv.setAttribute("class", "property");
        let elLabel = document.createElement('label');
        elLabel.setAttribute("for", serverScriptElementID + "-status");
        elLabel.innerText = "Server Script Status";
        let elServerScriptStatus = document.createElement('span');
        elServerScriptStatus.setAttribute("id", serverScriptElementID + "-status");
        elDiv.appendChild(elLabel);
        elDiv.appendChild(elServerScriptStatus);
        elServerScript.parentNode.appendChild(elDiv);
        
        // Server Script Error
        elDiv = document.createElement('div');
        elDiv.setAttribute("class", "property");
        let elServerScriptError = document.createElement('textarea');
        elServerScriptError.setAttribute("id", serverScriptElementID + "-error");
        elDiv.appendChild(elServerScriptError);
        elServerScript.parentNode.appendChild(elDiv);
        
        let elScript = getPropertyElement("script");
        elScript.parentNode.setAttribute("class", "property url refresh");
        elServerScript.parentNode.setAttribute("class", "property url refresh");
            
        // User Data
        let userDataProperty = properties["userData"];
        let elUserData = userDataProperty.el;
        let userDataElementID = userDataProperty.elementID;
        elDiv = elUserData.parentNode;
        let elStaticUserData = document.createElement('div');
        elStaticUserData.setAttribute("id", userDataElementID + "-static");
        let elUserDataEditor = document.createElement('div');
        elUserDataEditor.setAttribute("id", userDataElementID + "-editor");
        let elUserDataSaved = document.createElement('span');
        elUserDataSaved.setAttribute("id", userDataElementID + "-saved");
        elUserDataSaved.innerText = "Saved!";
        elDiv.childNodes[2].appendChild(elUserDataSaved);
        elDiv.insertBefore(elStaticUserData, elUserData);
        elDiv.insertBefore(elUserDataEditor, elUserData);
        
        // Material Data
        let materialDataProperty = properties["materialData"];
        let elMaterialData = materialDataProperty.el;
        let materialDataElementID = materialDataProperty.elementID;
        elDiv = elMaterialData.parentNode;
        let elStaticMaterialData = document.createElement('div');
        elStaticMaterialData.setAttribute("id", materialDataElementID + "-static");
        let elMaterialDataEditor = document.createElement('div');
        elMaterialDataEditor.setAttribute("id", materialDataElementID + "-editor");
        let elMaterialDataSaved = document.createElement('span');
        elMaterialDataSaved.setAttribute("id", materialDataElementID + "-saved");
        elMaterialDataSaved.innerText = "Saved!";
        elDiv.childNodes[2].appendChild(elMaterialDataSaved);
        elDiv.insertBefore(elStaticMaterialData, elMaterialData);
        elDiv.insertBefore(elMaterialDataEditor, elMaterialData);
        
        // User Data Fields
        let elGrabbable = getPropertyElement("grabbable");
        let elTriggerable = getPropertyElement("triggerable");
        let elIgnoreIK = getPropertyElement("ignoreIK");
        elGrabbable.addEventListener('change', function() {
            userDataChanger("grabbableKey", "grabbable", elGrabbable, elUserData, true);
        });
        elTriggerable.addEventListener('change', function() {
            userDataChanger("grabbableKey", "triggerable", elTriggerable, elUserData, false, ['wantsTrigger']);
        });
        elIgnoreIK.addEventListener('change', function() {
            userDataChanger("grabbableKey", "ignoreIK", elIgnoreIK, elUserData, true);
        });
        
        // Special Property Callbacks
        let elParentMaterialNameString = getPropertyElement("materialNameToReplace");
        let elParentMaterialNameNumber = getPropertyElement("submeshToReplace");
        let elParentMaterialNameCheckbox = getPropertyElement("selectSubmesh");
        elParentMaterialNameString.addEventListener('change', function () { 
            updateProperty("parentMaterialName", MATERIAL_PREFIX_STRING + this.value); 
        });
        elParentMaterialNameNumber.addEventListener('change', function () { 
            updateProperty("parentMaterialName", this.value); 
        });
        elParentMaterialNameCheckbox.addEventListener('change', function () {
            if (this.checked) {
                updateProperty("parentMaterialName", elParentMaterialNameNumber.value);
                showParentMaterialNameBox(true, elParentMaterialNameNumber, elParentMaterialNameString);
            } else {
                updateProperty("parentMaterialName", MATERIAL_PREFIX_STRING + elParentMaterialNameString.value);
                showParentMaterialNameBox(false, elParentMaterialNameNumber, elParentMaterialNameString);
            }
        });
        
        getPropertyElement("image").addEventListener('change', createImageURLUpdateFunction('textures'));
        
        // Collapsible sections
        let elCollapsible = document.getElementsByClassName("section-header");

        let toggleCollapsedEvent = function(event) {
            let element = event.target.parentNode.parentNode;
            let isCollapsed = element.dataset.collapsed !== "true";
            element.dataset.collapsed = isCollapsed ? "true" : false;
            element.setAttribute("collapsed", isCollapsed ? "true" : "false");
            element.getElementsByClassName(".collapse-icon")[0].textContent = isCollapsed ? "L" : "M";
        };

        for (let collapseIndex = 0, numCollapsibles = elCollapsible.length; collapseIndex < numCollapsibles; ++collapseIndex) {
            let curCollapsibleElement = elCollapsible[collapseIndex];
            curCollapsibleElement.getElementsByTagName('span')[0].addEventListener("click", toggleCollapsedEvent, true);
        }
        
        // Textarea scrollbars
        let elTextareas = document.getElementsByTagName("TEXTAREA");

        let textareaOnChangeEvent = function(event) {
            setTextareaScrolling(event.target);
        };

        for (let textAreaIndex = 0, numTextAreas = elTextareas.length; textAreaIndex < numTextAreas; ++textAreaIndex) {
            let curTextAreaElement = elTextareas[textAreaIndex];
            setTextareaScrolling(curTextAreaElement);
            curTextAreaElement.addEventListener("input", textareaOnChangeEvent, false);
            curTextAreaElement.addEventListener("change", textareaOnChangeEvent, false);
            /* FIXME: Detect and update textarea scrolling attribute on resize. Unfortunately textarea doesn't have a resize
            event; mouseup is a partial stand-in but doesn't handle resizing if mouse moves outside textarea rectangle. */
            curTextAreaElement.addEventListener("mouseup", textareaOnChangeEvent, false);
        }
        
        // Dropdowns
        // For each dropdown the following replacement is created in place of the original dropdown...
        // Structure created:
        //  <dl dropped="true/false">
        //      <dt name="?" id="?" value="?"><span>display text</span><span>carat</span></dt>
        //      <dd>
        //          <ul>
        //              <li value="??>display text</li>
        //              <li>...</li>
        //          </ul>
        //      </dd>
        //  </dl>    
        let elDropdowns = document.getElementsByTagName("select");
        for (let dropDownIndex = 0; dropDownIndex < elDropdowns.length; ++dropDownIndex) {
            let options = elDropdowns[dropDownIndex].getElementsByTagName("option");
            let selectedOption = 0;
            for (let optionIndex = 0; optionIndex < options.length; ++optionIndex) {
                if (options[optionIndex].getAttribute("selected") === "selected") {
                    selectedOption = optionIndex;
                    // TODO:  Shouldn't there be a break here?
                }
            }
            let div = elDropdowns[dropDownIndex].parentNode;

            let dl = document.createElement("dl");
            div.appendChild(dl);

            let dt = document.createElement("dt");
            dt.name = elDropdowns[dropDownIndex].name;
            dt.id = elDropdowns[dropDownIndex].id;
            dt.addEventListener("click", toggleDropdown, true);
            dl.appendChild(dt);

            let span = document.createElement("span");
            span.setAttribute("value", options[selectedOption].value);
            span.textContent = options[selectedOption].firstChild.textContent;
            dt.appendChild(span);

            let spanCaratDown = document.createElement("span");
            spanCaratDown.textContent = "5"; // caratDn
            dt.appendChild(spanCaratDown);

            let dd = document.createElement("dd");
            dl.appendChild(dd);

            let ul = document.createElement("ul");
            dd.appendChild(ul);

            for (let listOptionIndex = 0; listOptionIndex < options.length; ++listOptionIndex) {
                let li = document.createElement("li");
                li.setAttribute("value", options[listOptionIndex].value);
                li.textContent = options[listOptionIndex].firstChild.textContent;
                li.addEventListener("click", setDropdownValue);
                ul.appendChild(li);
            }
            
            let propertyID = elDropdowns[dropDownIndex].getAttribute("propertyID");
            let property = properties[propertyID];
            property.el = dt;
            dt.addEventListener('change', createEmitTextPropertyUpdateFunction(property.name));
        }
        
        elDropdowns = document.getElementsByTagName("select");
        while (elDropdowns.length > 0) {
            var el = elDropdowns[0];
            el.parentNode.removeChild(el);
            elDropdowns = document.getElementsByTagName("select");
        }
            
        document.addEventListener("keydown", function (keyDown) {
            if (keyDown.keyCode === KEY_P && keyDown.ctrlKey) {
                if (keyDown.shiftKey) {
                    EventBridge.emitWebEvent(JSON.stringify({ type: 'unparent' }));
                } else {
                    EventBridge.emitWebEvent(JSON.stringify({ type: 'parent' }));
                }
            }
        });
        
        window.onblur = function() {
            // Fake a change event
            let ev = document.createEvent("HTMLEvents");
            ev.initEvent("change", true, true);
            document.activeElement.dispatchEvent(ev);
        };
        
        // For input and textarea elements, select all of the text on focus
        let els = document.querySelectorAll("input, textarea");
        for (let i = 0; i < els.length; i++) {
            els[i].onfocus = function (e) {
                e.target.select();
            };
        }
        
        bindAllNonJSONEditorElements(); 

        showGroupsForType("None");
        resetProperties();
        disableProperties();        
    });

    augmentSpinButtons();

    // Disable right-click context menu which is not visible in the HMD and makes it seem like the app has locked
    document.addEventListener("contextmenu", function(event) {
        event.preventDefault();
    }, false);

    setTimeout(function() {
        EventBridge.emitWebEvent(JSON.stringify({ type: 'propertiesPageReady' }));
    }, 1000);
}
