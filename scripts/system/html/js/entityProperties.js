//  entityProperties.js
//
//  Created by Ryan Huffman on 13 Nov 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global alert, augmentSpinButtons, clearTimeout, console, document, Element, EventBridge, 
    HifiEntityUI, JSONEditor, openEventBridge, setTimeout, window, _ $ */
    
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
                propertyName: "type",
            },
            {
                label: "Name",
                type: "string",
                propertyName: "name",
            },
            {
                label: "ID",
                type: "string",
                propertyName: "id",
                readOnly: true,
            },
            {
                label: "Parent",
                type: "string",
                propertyName: "parentID",
            },
            {
                label: "Locked",
                glyph: "&#xe006;",
                type: "bool",
                propertyName: "locked",
            },
            {
                label: "Visible",
                glyph: "&#xe007;",
                type: "bool",
                propertyName: "visible",
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
                options: { Cube: "Box", Sphere: "Sphere", Tetrahedron: "Tetrahedron", Octahedron: "Octahedron", Icosahedron: "Icosahedron", Dodecahedron: "Dodecahedron", Hexagon: "Hexagon", Triangle: "Triangle", Octagon: "Octagon", Cylinder: "Cylinder", Cone: "Cone", Circle: "Circle", Quad: "Quad" },
                propertyName: "shape",
            },
            {
                label: "Color",
                type: "color",
                propertyName: "color",
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
                propertyName: "text",
            },
            {
                label: "Text Color",
                type: "color",
                propertyName: "textColor",
            },
            {
                label: "Background Color",
                type: "color",
                propertyName: "backgroundColor",
            },
            {
                label: "Line Height",
                type: "number",
                min: 0,
                step: 0.005,
                fixedDecimals: 4,
                unit: "m",
                propertyName: "lineHeight"
            },
            {
                label: "Face Camera",
                type: "bool",
                propertyName: "faceCamera"
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
                propertyName: "flyingAllowed",
            },
            {
                label: "Ghosting Allowed",
                type: "bool",
                propertyName: "ghostingAllowed",
            },
            {
                label: "Filter",
                type: "string",
                propertyName: "filterURL",
            },
            {
                label: "Key Light",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyName: "keyLightMode",
                
            },
            {
                label: "Key Light Color",
                type: "color",
                propertyName: "keyLight.color",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Light Intensity",
                type: "number",
                min: 0,
                max: 10,
                step: 0.1,
                fixedDecimals: 2,
                propertyName: "keyLight.intensity",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Light Altitude",
                type: "number",
                fixedDecimals: 2,
                unit: "deg",
                propertyName: "keyLight.direction.y",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Light Azimuth",
                type: "number",
                fixedDecimals: 2,
                unit: "deg",
                propertyName: "keyLight.direction.x",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Cast Shadows",
                type: "bool",
                propertyName: "keyLight.castShadows",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Skybox",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyName: "skyboxMode",
            },
            {
                label: "Skybox Color",
                type: "color",
                propertyName: "skybox.color",
                showPropertyRule: { "skyboxMode": "enabled" },
            },
            {
                label: "Skybox URL",
                type: "string",
                propertyName: "skybox.url",
                showPropertyRule: { "skyboxMode": "enabled" },
            },
            {
                type: "buttons",
                buttons: [ { id: "copy", label: "Copy URL To Ambient", className: "black", onClick: copySkyboxURLToAmbientURL } ],
                propertyName: "copyURLToAmbient",
                showPropertyRule: { "skyboxMode": "enabled" },
            },
            {
                label: "Ambient Light",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyName: "ambientLightMode",
            },
            {
                label: "Ambient Intensity",
                type: "number",
                min: 0,
                max: 10,
                step: 0.1,
                fixedDecimals: 2,
                propertyName: "ambientLight.ambientIntensity",
                showPropertyRule: { "ambientLightMode": "enabled" },
            },
            {
                label: "Ambient URL",
                type: "string",
                propertyName: "ambientLight.ambientURL",
                showPropertyRule: { "ambientLightMode": "enabled" },
            },
            {
                label: "Haze",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyName: "hazeMode",
            },
            {
                label: "Range",
                type: "number",
                min: 5,
                max: 10000,
                step: 5,
                fixedDecimals: 0,
                unit: "m",
                propertyName: "haze.hazeRange",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Use Altitude",
                type: "bool",
                propertyName: "haze.hazeAltitudeEffect",
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
                propertyName: "haze.hazeBaseRef",
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
                propertyName: "haze.hazeCeiling",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Haze Color",
                type: "color",
                propertyName: "haze.hazeColor",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Background Blend",
                type: "number",
                min: 0.0,
                max: 1.0,
                step: 0.01,
                fixedDecimals: 2,
                propertyName: "haze.hazeBackgroundBlend",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Enable Glare",
                type: "bool",
                propertyName: "haze.hazeEnableGlare",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Glare Color",
                type: "color",
                propertyName: "haze.hazeGlareColor",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Glare Angle",
                type: "number",
                min: 0,
                max: 180,
                step: 1,
                fixedDecimals: 0,
                propertyName: "haze.hazeGlareAngle",
                showPropertyRule: { "hazeMode": "enabled" },
            },
            {
                label: "Bloom",
                type: "dropdown",
                options: { inherit: "Inherit", disabled: "Off", enabled: "On" },
                propertyName: "bloomMode",
            },
            {
                label: "Bloom Intensity",
                type: "number",
                min: 0,
                max: 1,
                step: 0.01,
                fixedDecimals: 2,
                propertyName: "bloom.bloomIntensity",
                showPropertyRule: { "bloomMode": "enabled" },
            },
            {
                label: "Bloom Threshold",
                type: "number",
                min: 0,
                min: 1,
                step: 0.01,
                fixedDecimals: 2,
                propertyName: "bloom.bloomThreshold",
                showPropertyRule: { "bloomMode": "enabled" },
            },
            {
                label: "Bloom Size",
                type: "number",
                min: 0,
                min: 2,
                step: 0.01,
                fixedDecimals: 2,
                propertyName: "bloom.bloomSize",
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
                propertyName: "modelURL",
            },
            {
                label: "Collision Shape",
                type: "dropdown",
                options: { "none": "No Collision", "box": "Box", "sphere": "Sphere", "compound": "Compound" , "simple-hull": "Basic - Whole model", "simple-compound": "Good - Sub-meshes" , "static-mesh": "Exact - All polygons (non-dynamic only)" },
                propertyName: "shapeType",
            },
            {
                label: "Compound Shape",
                type: "string",
                propertyName: "compoundShapeURL",
            },
            {
                label: "Animation",
                type: "string",
                propertyName: "animation.url",
            },
            {
                label: "Play Automatically",
                type: "bool",
                propertyName: "animation.running",
            },
            {
                label: "Allow Transition",
                type: "bool",
                propertyName: "animation.allowTranslation",
            },
            {
                label: "Loop",
                type: "bool",
                propertyName: "animation.loop",
            },
            {
                label: "Hold",
                type: "bool",
                propertyName: "animation.hold",
            },
            {
                label: "Animation Frame",
                type: "number",
                propertyName: "animation.currentFrame",
            },
            {
                label: "First Frame",
                type: "number",
                propertyName: "animation.firstFrame",
            },
            {
                label: "Last Frame",
                type: "number",
                propertyName: "animation.lastFrame",
            },
            {
                label: "Animation FPS",
                type: "number",
                propertyName: "animation.fps",
            },
            {
                label: "Texture",
                type: "textarea",
                propertyName: "textures",
            },
            {
                label: "Original Texture",
                type: "textarea",
                propertyName: "originalTextures",
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
                propertyName: "image",
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
                propertyName: "sourceUrl",
            },
            {
                label: "Source Resolution",
                type: "number",
                propertyName: "dpi",
            },
        ]
    },
    {
        id: "light",
        addToGroup: "base",
        properties: [
            {
                label: "Light Color",
                type: "color",
                propertyName: "lightColor", // this actually shares "color" property with shape Color
                                            // but separating naming here to separate property fields
            },
            {
                label: "Intensity",
                type: "number",
                min: 0,
                step: 0.1,
                fixedDecimals: 1,
                propertyName: "intensity",
            },
            {
                label: "Fall-Off Radius",
                type: "number",
                min: 0,
                step: 0.1,
                fixedDecimals: 1,
                unit: "m",
                propertyName: "falloffRadius",
            },
            {
                label: "Spotlight",
                type: "bool",
                propertyName: "isSpotlight",
            },
            {
                label: "Spotlight Exponent",
                type: "number",
                step: 0.01,
                fixedDecimals: 2,
                propertyName: "exponent",
            },
            {
                label: "Spotlight Cut-Off",
                type: "number",
                step: 0.01,
                fixedDecimals: 2,
                propertyName: "cutoff",
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
                propertyName: "materialURL",
            },
            {
                label: "Material Data",
                type: "textarea",
                buttons: [ { id: "clear", label: "Clear Material Data", className: "red", onClick: clearMaterialData }, 
                            { id: "edit", label: "Edit as JSON", className: "blue", onClick: newJSONMaterialEditor },
                            { id: "save", label: "Save Material Data", className: "black", onClick: saveMaterialData } ],
                propertyName: "materialData",
            },
            {
                label: "Submesh to Replace",
                type: "number",
                min: 0,
                step: 1,
                propertyName: "submeshToReplace",
            },
            {
                label: "Material Name to Replace",
                type: "string",
                propertyName: "materialNameToReplace",
            },
            {
                label: "Select Submesh",
                type: "bool",
                propertyName: "selectSubmesh",
            },
            {
                label: "Priority",
                type: "number",
                min: 0,
                propertyName: "priority",
            },
            {
                label: "Material Position",
                type: "vec2",
                min: 0,
                min: 1,
                step: 0.1,
                vec2Type: "xy",
                subLabels: [ "x", "y" ],
                propertyName: "materialMappingPos",
            },
            {
                label: "Material Scale",
                type: "vec2",
                min: 0,
                step: 0.1,
                vec2Type: "wh",
                subLabels: [ "width", "height" ],
                propertyName: "materialMappingScale",
            },
            {
                label: "Material Rotation",
                type: "number",
                step: 0.1,
                fixedDecimals: 2,
                unit: "deg",
                propertyName: "materialMappingRot",
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
                propertyName: "position",
            },
            {
                label: "Rotation",
                type: "vec3",
                step: 0.1,
                vec3Type: "pyr",
                subLabels: [ "pitch", "yaw", "roll" ],
                unit: "deg",
                propertyName: "rotation",
            },
            {
                label: "Dimension",
                type: "vec3",
                step: 0.1,
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "m",
                propertyName: "dimensions",
            },
            {
                label: "Scale",
                type: "number",
                defaultValue: 100,
                unit: "%",
                buttons: [ { id: "rescale", label: "Rescale", className: "blue", onClick: rescaleDimensions }, 
                            { id: "reset", label: "Reset Dimensions", className: "red", onClick: resetToNaturalDimensions } ],
                propertyName: "scale",
            },
            {
                label: "Pivot",
                type: "vec3",
                step: 0.1,
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "(ratio of dimension)",
                propertyName: "registrationPoint",
            },
            {
                label: "Align",
                type: "buttons",
                buttons: [ { id: "selection", label: "Selection to Grid", className: "black", onClick: moveSelectionToGrid }, 
                            { id: "all", label: "All to Grid", className: "black", onClick: moveAllToGrid } ],
                propertyName: "alignToGrid",
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
                propertyName: "collisionless",
                inverse: true,
                column: -1, // before two columns div
            },
            {
                label: "Dynamic",
                type: "bool",
                propertyName: "dynamic",
                column: -1, // before two columns div
            },
            {
                label: "Collides With",
                type: "sub-header",
                propertyName: "collidesWithHeader",
                showPropertyRule: { "collisionless": "false" },
                column: 1,
            },
            {
                label: "",
                type: "sub-header",
                propertyName: "collidesWithHeaderHelper",
                showPropertyRule: { "collisionless": "false" },
                column: 2,
            },
            {
                label: "Static Entities",
                type: "bool",
                propertyName: "static",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 1,
            },
            {
                label: "Dynamic Entities",
                type: "bool",
                propertyName: "dynamic",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 2,
            },
            {
                label: "Kinematic Entities",
                type: "bool",
                propertyName: "kinematic",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 1,
            },
            {
                label: "My Avatar",
                type: "bool",
                propertyName: "myAvatar",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 2,
            },
            {
                label: "Other Avatars",
                type: "bool",
                propertyName: "otherAvatar",
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
                column: 1,
            },
            {
                label: "Collision sound URL",
                type: "string",
                propertyName: "collisionSoundURL",
                showPropertyRule: { "collisionless": "false" },
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
                propertyName: "grabbable",
                column: 1,
            },
            {
                label: "Triggerable",
                type: "bool",
                propertyName: "triggerable",
                column: 2,
            },
            {
                label: "Cloneable",
                type: "bool",
                propertyName: "cloneable",
                column: 1,
            },
            {
                label: "Ignore inverse kinematics",
                type: "bool",
                propertyName: "ignoreIK",
                column: 2,
            },
            {
                label: "Clone Lifetime",
                type: "number",
                unit: "s",
                propertyName: "cloneLifetime",
                showPropertyRule: { "cloneable": "true" },
                column: 1,
            },
            {
                label: "Clone Limit",
                type: "number",
                propertyName: "cloneLimit",
                showPropertyRule: { "cloneable": "true" },
                column: 1,
            },
            {
                label: "Clone Dynamic",
                type: "bool",
                propertyName: "cloneDynamic",
                showPropertyRule: { "cloneable": "true" },
                column: 1,
            },
            {
                label: "Clone Avatar Entity",
                type: "bool",
                propertyName: "cloneAvatarEntity",
                showPropertyRule: { "cloneable": "true" },
                column: 1,
            },
            {
                label: "Can cast shadow",
                type: "bool",
                propertyName: "canCastShadow",
            },
            {
                label: "Script",
                type: "string",
                buttons: [ { id: "reload", label: "F", className: "glyph", onClick: reloadScripts } ],
                propertyName: "script",
            },
            {
                label: "Server Script",
                type: "string",
                buttons: [ { id: "reload", label: "F", className: "glyph", onClick: reloadServerScripts } ],
                propertyName: "serverScripts",
            },
            {
                label: "Lifetime",
                type: "number",
                unit: "s",
                propertyName: "lifetime",
            },
            {
                label: "User Data",
                type: "textarea",
                buttons: [ { id: "clear", label: "Clear User Data", className: "red", onClick: clearUserData }, 
                            { id: "edit", label: "Edit as JSON", className: "blue", onClick: newJSONEditor },
                            { id: "save", label: "Save User Data", className: "black", onClick: saveUserData } ],
                propertyName: "userData",
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
                propertyName: "velocity",
            },
            {
                label: "Linear Damping",
                type: "number",
                fixedDecimals: 2,
                propertyName: "damping",
            },
            {
                label: "Angular Velocity",
                type: "vec3",
                multiplier: DEGREES_TO_RADIANS,
                vec3Type: "pyr",
                subLabels: [ "pitch", "yaw", "roll" ],
                unit: "deg/s",
                propertyName: "angularVelocity",
            },
            {
                label: "Angular Damping",
                type: "number",
                fixedDecimals: 4,
                propertyName: "angularDamping",
            },
            {
                label: "Bounciness",
                type: "number",
                fixedDecimals: 4,
                propertyName: "restitution",
            },
            {
                label: "Friction",
                type: "number",
                fixedDecimals: 4,
                propertyName: "friction",
            },
            {
                label: "Density",
                type: "number",
                fixedDecimals: 4,
                propertyName: "density",
            },
            {
                label: "Gravity",
                type: "vec3",
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "m/s<sup>2</sup>",
                propertyName: "gravity",
            },
            {
                label: "Acceleration",
                type: "vec3",
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                unit: "m/s<sup>2</sup>",
                propertyName: "acceleration",
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
var elPropertyElements = {};
var showPropertyRules = {};
var subProperties = [];
var icons = {};
var colorPickers = {};
var lastEntityID = null;

function debugPrint(message) {
    EventBridge.emitWebEvent(
        JSON.stringify({
            type: "print",
            message: message
        })
    );
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
    var elLocked = elPropertyElements["locked"];

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
    var elLocked = elPropertyElements["locked"];

    if (elLocked.checked === true) {
        if ($('#property-userData-editor').css('display') === "block") {
            showStaticUserData();
        }
        if ($('#property-materialData-editor').css('display') === "block") {
            showStaticMaterialData();
        }
    }
}

function updateProperty(propertyName, propertyValue) {
    var properties = {};
    let splitPropertyName = propertyName.split('.');
    if (splitPropertyName.length > 1) {
        let propertyGroupName = splitPropertyName[0];
        let subPropertyName = splitPropertyName[1];
        properties[propertyGroupName] = {};
        if (splitPropertyName.length === 3) { 
            let subSubPropertyName = splitPropertyName[2];
            properties[propertyGroupName][subPropertyName] = {};
            properties[propertyGroupName][subPropertyName][subSubPropertyName] = propertyValue;
        } else {
            properties[propertyGroupName][subPropertyName] = propertyValue;
        }
    } else {
        properties[propertyName] = propertyValue;
    }
    updateProperties(properties);
}

function updateProperties(properties) {
    EventBridge.emitWebEvent(JSON.stringify({
        id: lastEntityID,
        type: "update",
        properties: properties
    }));
}

function createEmitCheckedPropertyUpdateFunction(propertyName, inverse) {
    return function() {
        updateProperty(propertyName, inverse ? !this.checked : this.checked);
    };
}

function createEmitNumberPropertyUpdateFunction(propertyName, decimals) {
    decimals = ((decimals === undefined) ? 4 : decimals);
    return function() {
        var value = parseFloat(this.value).toFixed(decimals);
        updateProperty(propertyName, value);
    };
}

function createImageURLUpdateFunction(propertyName) {
    return function () {
        var newTextures = JSON.stringify({ "tex.picture": this.value });
        updateProperty(propertyName, newTextures);
    };
}

function createEmitTextPropertyUpdateFunction(propertyName) {
    return function() {
        updateProperty(propertyName, this.value);
    };
}

function createEmitVec2PropertyUpdateFunction(property, elX, elY) {
    return function () {
        var properties = {};
        properties[property] = {
            x: elX.value,
            y: elY.value
        };
        updateProperties(properties);
    };
}

function createEmitVec2PropertyUpdateFunctionWithMultiplier(property, elX, elY, multiplier) {
    return function () {
        var properties = {};
        properties[property] = {
            x: elX.value * multiplier,
            y: elY.value * multiplier
        };
        updateProperties(properties);
    };
}

function createEmitVec3PropertyUpdateFunction(property, elX, elY, elZ) {
    return function() {
        var properties = {};
        properties[property] = {
            x: elX.value,
            y: elY.value,
            z: elZ ? elZ.value : 0
        };
        updateProperties(properties);
    };
}

function createEmitVec3PropertyUpdateFunctionWithMultiplier(property, elX, elY, elZ, multiplier) {
    return function() {
        var properties = {};
        properties[property] = {
            x: elX.value * multiplier,
            y: elY.value * multiplier,
            z: elZ.value * multiplier
        };
        updateProperties(properties);
    };
}

function createEmitColorPropertyUpdateFunction(property, elRed, elGreen, elBlue) {
    return function() {
        emitColorPropertyUpdate(property, elRed.value, elGreen.value, elBlue.value);
    };
}

function emitColorPropertyUpdate(property, red, green, blue, group) {
    var properties = {};
    if (group) {
        properties[group] = {};
        properties[group][property] = {
            red: red,
            green: green,
            blue: blue
        };
    } else {
        properties[property] = {
            red: red,
            green: green,
            blue: blue
        };
    }
    updateProperties(properties);
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

function clearUserData() {
    let elUserData = elPropertyElements["userData"];
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
    var properties = {};
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
        properties.userData = JSON.stringify(parsedData);
    } else {
        properties.userData = '';
    }

    userDataElement.value = properties.userData;

    updateProperties(properties);
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

function clearMaterialData() {
    let elMaterialData = elPropertyElements["materialData"];
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
            if (e.target.id === "property-userData-button-edit" || e.target.id === "property-userData-button-clear" || e.target.id === "property-materialData-button-edit" || e.target.id === "property-materialData-button-clear") {
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

function unbindAllInputs() {
    var inputs = $('input');
    var i;
    for (i = 0; i < inputs.length; i++) {
        var input = inputs[i];
        var field = $(input);
        field.unbind();
    }
}

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
    let skyboxURL = elPropertyElements["skybox.url"].value;
    elPropertyElements["ambientLight.ambientURL"].value = skyboxURL;
    updateProperty("ambientLight.ambientURL", skyboxURL);
}

function createTupleNumberInput(elTuple, propertyID, subLabel, min, max, step) {
    let elementPropertyID = propertyID + "-" + subLabel.toLowerCase();
    let elDiv = document.createElement('div');
    let elLabel = document.createElement('label');
    elLabel.innerText = subLabel[0].toUpperCase() + subLabel.slice(1) + ":";
    elLabel.setAttribute("for", elementPropertyID);
    let elInput = document.createElement('input');
    elInput.setAttribute("id", elementPropertyID);
    elInput.setAttribute("type", "number");
    elInput.setAttribute("class", subLabel);
    if (min !== undefined) {
        elInput.setAttribute("min", min);
    }
    if (max !== undefined) {
        elInput.setAttribute("max", max);
    }
    if (step !== undefined) {
        elInput.setAttribute("step", step);
    }
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

function resetProperties() {
    for (let propertyName in elPropertyElements) {
        let elProperty = elPropertyElements[propertyName];                          
        if (elProperty instanceof Array) {
            if (elProperty.length === 2 || elProperty.length === 3) {
                // vec2/vec3 are array of 2/3 input numbers
                elProperty[0].value = "";
                elProperty[1].value = "";
                if (elProperty[2] !== undefined) {
                    elProperty[2].value = "";
                }
            } else if (elProperty.length === 4) {
                // color is array of color picker and 3 input numbers
                elProperty[0].style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                elProperty[1].value = "";
                elProperty[2].value = "";
                elProperty[3].value = "";
            }
        } else if (elProperty.getAttribute("type") === "number") {
            if (elProperty.getAttribute("defaultValue")) {
                elProperty.value = elProperty.getAttribute("defaultValue");
            } else { 
                elProperty.value = "";
            }
        } else if (elProperty.getAttribute("type") === "checkbox") {
            elProperty.checked = false;
        } else {
            elProperty.value = "";
        }
    }
    
    for (let showPropertyRule in showPropertyRules) {
        let propertyShowRules = showPropertyRules[showPropertyRule];
        for (let propertyToHide in propertyShowRules) {
            let elPropertyToHide = elPropertyElements[propertyToHide];
            if (elPropertyToHide) {
                let nodeToHide = elPropertyToHide;
                if (elPropertyToHide.nodeName !== "DIV") {
                    let parentNode = elPropertyToHide.parentNode;
                    if (parentNode === undefined && elPropertyToHide instanceof Array) {
                        parentNode = elPropertyToHide[0].parentNode;
                    }
                    if (parentNode !== undefined) {
                        nodeToHide = parentNode;
                    }
                }
                nodeToHide.style.display = "none";
            }
        }
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
                
            group.properties.forEach(function(property) {
                let propertyType = property.type;
                let propertyName = property.propertyName;
                let propertyID = "property-" + propertyName;
                propertyID = propertyID.replace(".", "-");
                
                let elProperty;
                if (propertyType === "sub-header") {
                    elProperty = document.createElement('legend');
                    elProperty.innerText = property.label;
                    elProperty.setAttribute("class", "sub-section-header");
                } else {
                    elProperty = document.createElement('div');
                    elProperty.setAttribute("id", "div-" + propertyID);
                }
                
                if (group.twoColumn && property.column !== undefined && property.column !== -1) {
                    let columnName = group.id + "column" + property.column;
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
                elLabel.innerText = property.label;
                elLabel.setAttribute("for", propertyID);
                
                switch (propertyType) {
                    case 'vec3': {
                        elProperty.setAttribute("class", "property " + property.vec3Type + " fstuple");
                        
                        let elTuple = document.createElement('div');
                        elTuple.setAttribute("class", "tuple");
                        
                        addUnit(property.unit, elLabel);
                        
                        elProperty.appendChild(elLabel);
                        elProperty.appendChild(elTuple);
                        
                        let elInputX = createTupleNumberInput(elTuple, propertyID, property.subLabels[0], property.min, property.max, property.step);
                        let elInputY = createTupleNumberInput(elTuple, propertyID, property.subLabels[1], property.min, property.max, property.step);
                        let elInputZ = createTupleNumberInput(elTuple, propertyID, property.subLabels[2], property.min, property.max, property.step);
                        
                        let inputChangeFunction;
                        let multiplier = 1;
                        if (property.multiplier !==  undefined) { 
                            multiplier = property.multiplier;
                            inputChangeFunction = createEmitVec3PropertyUpdateFunction(propertyName, elInputX, elInputY, elInputZ);
                        } else {
                            inputChangeFunction = createEmitVec3PropertyUpdateFunctionWithMultiplier(propertyName, elInputX, elInputY, elInputZ, property.multiplier);
                        }
                        elInputX.setAttribute("multiplier", multiplier);
                        elInputY.setAttribute("multiplier", multiplier);
                        elInputZ.setAttribute("multiplier", multiplier);
                        elInputX.addEventListener('change', inputChangeFunction);
                        elInputY.addEventListener('change', inputChangeFunction);
                        elInputZ.addEventListener('change', inputChangeFunction);
                        
                        elPropertyElements[propertyName] = [elInputX, elInputY, elInputZ];
                        break;
                    }
                    case 'vec2': {
                        elProperty.setAttribute("class", "property " + property.vec2Type + " fstuple");
                        
                        let elTuple = document.createElement('div');
                        elTuple.setAttribute("class", "tuple");
                        
                        addUnit(property.unit, elLabel);
                        
                        elProperty.appendChild(elLabel);
                        elProperty.appendChild(elTuple);
                        
                        let elInputX = createTupleNumberInput(elTuple, propertyID, property.subLabels[0], property.min, property.max, property.step);
                        let elInputY = createTupleNumberInput(elTuple, propertyID, property.subLabels[1], property.min, property.max, property.step);
                        
                        let inputChangeFunction;    
                        let multiplier = 1;
                        if (property.multiplier !==  undefined) { 
                            multiplier = property.multiplier;
                            inputChangeFunction = createEmitVec2PropertyUpdateFunction(propertyName, elInputX, elInputY);
                        } else {
                            inputChangeFunction = createEmitVec2PropertyUpdateFunctionWithMultiplier(propertyName, elInputX, elInputY, property.multiplier);
                        }
                        elInputX.setAttribute("multiplier", multiplier);
                        elInputY.setAttribute("multiplier", multiplier);
                        elInputX.addEventListener('change', inputChangeFunction);
                        elInputY.addEventListener('change', inputChangeFunction);
                        
                        elPropertyElements[propertyName] = [elInputX, elInputY];
                        break;
                    }
                    case 'color': {
                        elProperty.setAttribute("class", "property rgb fstuple");
                        
                        let elColorPicker = document.createElement('div');
                        elColorPicker.setAttribute("class", "color-picker");
                        elColorPicker.setAttribute("id", propertyID);
                        
                        let elTuple = document.createElement('div');
                        elTuple.setAttribute("class", "tuple");
                        
                        elProperty.appendChild(elColorPicker);
                        elProperty.appendChild(elLabel);
                        elProperty.appendChild(elTuple);
                        
                        let elInputR = createTupleNumberInput(elTuple, propertyID, "red", 0, 255, 1);
                        let elInputG = createTupleNumberInput(elTuple, propertyID, "green", 0, 255, 1);
                        let elInputB = createTupleNumberInput(elTuple, propertyID, "blue", 0, 255, 1);
                        
                        let inputChangeFunction = createEmitColorPropertyUpdateFunction(propertyName, elInputR, elInputG, elInputB);    
                        elInputR.addEventListener('change', inputChangeFunction);
                        elInputG.addEventListener('change', inputChangeFunction);
                        elInputB.addEventListener('change', inputChangeFunction);
                        
                        let colorPickerID = "#" + propertyID;
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
                                emitColorPropertyUpdate('color', rgb.r, rgb.g, rgb.b);
                            }
                        });
                        
                        elPropertyElements[propertyName] = [elColorPicker, elInputR, elInputG, elInputB];
                        break;
                    }
                    case 'string': {
                        elProperty.setAttribute("class", "property text");
                        
                        let elInput = document.createElement('input');
                        elInput.setAttribute("id", propertyID);
                        elInput.setAttribute("type", "text"); 
                        if (property.readOnly) {
                            elInput.readOnly = true;
                        }
                        
                        elInput.addEventListener('change', createEmitTextPropertyUpdateFunction(propertyName));
                        
                        elProperty.appendChild(elLabel);
                        elProperty.appendChild(elInput);
                        
                        if (property.buttons !== undefined) {
                            addButtons(elProperty, propertyID, property.buttons, false);
                        }
                        
                        elPropertyElements[propertyName] = elInput;
                        break;
                    }
                    case 'bool': {
                        elProperty.setAttribute("class", "property checkbox");
                        
                        if (property.glyph !== undefined) {
                            elLabel.innerText = " " + property.label;
                            let elSpan = document.createElement('span');
                            elSpan.innerHTML = property.glyph;
                            elLabel.insertBefore(elSpan, elLabel.firstChild);
                        }
                        
                        let elInput = document.createElement('input');
                        elInput.setAttribute("id", propertyID);
                        elInput.setAttribute("type", "checkbox");
                        
                        let inverse = property.inverse;
                        elInput.setAttribute("inverse", inverse ? "true" : "false");
                        
                        elProperty.appendChild(elInput);
                        elProperty.appendChild(elLabel);
                        
                        let subPropertyOf = property.subPropertyOf;
                        if (subPropertyOf !== undefined) {
                            elInput.setAttribute("subPropertyOf", subPropertyOf);
                            elPropertyElements[propertyName] = elInput;
                            subProperties.push(propertyName);
                            elInput.addEventListener('change', function() {
                                updateCheckedSubProperty(subPropertyOf, properties[subPropertyOf], elInput, propertyName);
                            });
                        } else {
                            elPropertyElements[propertyName] = elInput;
                            elInput.addEventListener('change', createEmitCheckedPropertyUpdateFunction(propertyName, inverse));
                        }
                        break;
                    }
                    case 'dropdown': {
                        elProperty.setAttribute("class", "property dropdown");
                        
                        let elInput = document.createElement('select');
                        elInput.setAttribute("id", propertyID);
                        elInput.setAttribute("propertyName", propertyName);
                        
                        for (let optionKey in property.options) {
                            let option = document.createElement('option');
                            option.value = optionKey;
                            option.text = property.options[optionKey];
                            elInput.add(option);
                        }
                        
                        elInput.addEventListener('change', createEmitTextPropertyUpdateFunction(propertyName));
                        
                        elProperty.appendChild(elLabel);
                        elProperty.appendChild(elInput);
                        elPropertyElements[propertyName] = elInput;
                        break;
                    }
                    case 'number': {
                        elProperty.setAttribute("class", "property number");
                        
                        addUnit(property.unit, elLabel);
                        
                        let elInput = document.createElement('input');
                        elInput.setAttribute("id", propertyID);
                        elInput.setAttribute("type", "number");
                        if (property.min !== undefined) {
                            elInput.setAttribute("min", property.min);
                        }
                        if (property.max !== undefined) {
                            elInput.setAttribute("max", property.max);
                        }
                        if (property.step !== undefined) {
                            elInput.setAttribute("step", property.step);
                        }
                        
                        let fixedDecimals = property.fixedDecimals !== undefined ? property.fixedDecimals : 0;
                        elInput.setAttribute("fixedDecimals", fixedDecimals);
                        
                        let defaultValue = property.defaultValue;
                        if (defaultValue !== undefined) {
                            elInput.setAttribute("defaultValue", defaultValue);
                            elInput.value = defaultValue;
                        }
                        
                        elInput.addEventListener('change', createEmitNumberPropertyUpdateFunction(propertyName, fixedDecimals));
                        
                        elProperty.appendChild(elLabel);
                        elProperty.appendChild(elInput);
                        
                        if (property.buttons !== undefined) {
                            addButtons(elProperty, propertyID, property.buttons, true);
                        }
                        
                        elPropertyElements[propertyName] = elInput;
                        break;
                    }
                    case 'textarea': {
                        elProperty.setAttribute("class", "property textarea");
                        
                        elProperty.appendChild(elLabel);
                        
                        if (property.buttons !== undefined) {
                            addButtons(elProperty, propertyID, property.buttons, true);
                        }
                        
                        let elInput = document.createElement('textarea');
                        elInput.setAttribute("id", propertyID);
                        if (property.readOnly) {
                            elInput.readOnly = true;
                        }                   
                        
                        elInput.addEventListener('change', createEmitTextPropertyUpdateFunction(propertyName));
                        
                        elProperty.appendChild(elInput);
                        elPropertyElements[propertyName] = elInput;
                        break;
                    }
                    case 'icon': {
                        elProperty.setAttribute("class", "property value");
                        
                        elLabel.setAttribute("id", propertyID);
                        elLabel.innerHTML = " " + property.label;
                        
                        let elSpan = document.createElement('span');
                        elSpan.setAttribute("id", propertyID + "-icon");
                        icons[propertyName] = property.icons;

                        elProperty.appendChild(elSpan);
                        elProperty.appendChild(elLabel);
                        elPropertyElements[propertyName] = [ elSpan, elLabel ];
                        break;
                    }
                    case 'buttons': {
                        elProperty.setAttribute("class", "property text");
                        
                        let hasLabel = property.label !== undefined;
                        if (hasLabel) {
                            elProperty.appendChild(elLabel);
                        }
                        
                        if (property.buttons !== undefined) {
                            addButtons(elProperty, propertyID, property.buttons, hasLabel);
                        }
                        
                        elPropertyElements[propertyName] = elProperty;
                        break;
                    }
                    case 'sub-header': {
                        if (propertyName !== undefined) {
                            elPropertyElements[propertyName] = elProperty;
                        }
                        break;
                    }
                }
                
                let showPropertyRule = property.showPropertyRule;
                if (showPropertyRule !== undefined) {
                    let dependentProperty = Object.keys(showPropertyRule)[0];
                    let dependentPropertyValue = showPropertyRule[dependentProperty];
                    if (!showPropertyRules[dependentProperty]) {
                        showPropertyRules[dependentProperty] = [];
                    }
                    showPropertyRules[dependentProperty][propertyName] = dependentPropertyValue;
                }
            });
            
            elGroups[group.id] = elGroup;
        });
        
        if (window.EventBridge !== undefined) {
            var properties;
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
                        
                        elPropertyElements["type"][0].style.display = "none";
                        elPropertyElements["type"][1].innerHTML = NO_SELECTION;
                        elPropertiesList.className = '';
                        showGroupsForType("None");
                        
                        resetProperties();
                
                        deleteJSONEditor();
                        elPropertyElements["userData"].value = "";
                        showUserDataTextArea();
                        showSaveUserDataButton();
                        showNewJSONEditorButton();
                        
                        deleteJSONMaterialEditor();
                        elPropertyElements["materialData"].value = "";
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
                        
                        elPropertyElements["type"][0].innerHTML = ICON_FOR_TYPE[type];
                        elPropertyElements["type"][0].style.display = "inline-block";
                        elPropertyElements["type"][1].innerHTML = type + " (" + data.selections.length + ")";
                        elPropertiesList.className = '';
                        showGroupsForType(type);
                        
                        resetProperties();
                        disableProperties();
                    } else {
                        properties = data.selections[0].properties;
                        
                        if (lastEntityID !== '"' + properties.id + '"' && lastEntityID !== null) {
                            if (editor !== null) {
                               saveUserData();
                            }
                            if (materialEditor !== null) {
                                saveMaterialData();
                            }
                        }

                        let doSelectElement = lastEntityID === '"' + properties.id + '"';

                        // the event bridge and json parsing handle our avatar id string differently.
                        lastEntityID = '"' + properties.id + '"';

                        // HTML workaround since image is not yet a separate entity type
                        let IMAGE_MODEL_NAME = 'default-image-model.fbx';
                        if (properties.type === "Model") {
                            let urlParts = properties.modelURL.split('/');
                            let propsFilename = urlParts[urlParts.length - 1];

                            if (propsFilename === IMAGE_MODEL_NAME) {
                                properties.type = "Image";
                            }
                        }
                        
                        // Create class name for css ruleset filtering
                        elPropertiesList.className = properties.type + 'Menu';
                        showGroupsForType(properties.type);
                        
                        for (let propertyName in elPropertyElements) {
                            let elProperty = elPropertyElements[propertyName];
                            
                            let propertyValue;
                            let splitPropertyName = propertyName.split('.');
                            if (splitPropertyName.length > 1) {
                                let propertyGroupName = splitPropertyName[0];
                                let subPropertyName = splitPropertyName[1];
                                if (properties[propertyGroupName] === undefined || properties[propertyGroupName][subPropertyName] === undefined) {
                                    continue;
                                }
                                if (splitPropertyName.length === 3) { 
                                    let subSubPropertyName = splitPropertyName[2];
                                    propertyValue = properties[propertyGroupName][subPropertyName][subSubPropertyName];
                                } else {
                                    propertyValue = properties[propertyGroupName][subPropertyName];
                                }
                            } else {
                                propertyValue = properties[propertyName];
                            }

                            // workaround for shape Color & Light Color property fields sharing same property value "color"
                            if (propertyValue === undefined && propertyName === "lightColor") {
                                propertyValue = properties["color"]
                            }
                            
                            let isSubProperty = subProperties.indexOf(propertyName) > -1;
                            if (elProperty === undefined || (propertyValue === undefined && !isSubProperty)) {
                                continue;
                            }
                            
                            if (elProperty instanceof Array) {
                                if (elProperty[1].getAttribute("type") === "number") { // vectors
                                    if (elProperty.length === 2 || elProperty.length === 3) {
                                        // vec2/vec3 are array of 2/3 input numbers
                                        elProperty[0].value = (propertyValue.x * 1 / elProperty[0].getAttribute("multiplier")).toFixed(4);
                                        elProperty[1].value = (propertyValue.y * 1 / elProperty[1].getAttribute("multiplier")).toFixed(4);
                                        if (elProperty[2] !== undefined) {
                                            elProperty[2].value = (propertyValue.z * 1 / elProperty[2].getAttribute("multiplier")).toFixed(4);
                                        }
                                    } else if (elProperty.length === 4) {
                                        // color is array of color picker and 3 input numbers
                                        elProperty[0].style.backgroundColor = "rgb(" + propertyValue.red + "," + propertyValue.green + "," + propertyValue.blue + ")";
                                        elProperty[1].value = propertyValue.red;
                                        elProperty[2].value = propertyValue.green;
                                        elProperty[3].value = propertyValue.blue;
                                    }
                                } else if (elProperty[0].nodeName === "SPAN") { // icons
                                    // icon is array of elSpan (icon glyph) and elLabel
                                    elProperty[0].innerHTML = icons[propertyName][propertyValue];
                                    elProperty[0].style.display = "inline-block";
                                    elProperty[1].innerHTML = propertyValue; //+ " (" + data.selections.length + ")";
                                }
                            } else if (elProperty.getAttribute("subPropertyOf")) {
                                let propertyValue = properties[elProperty.getAttribute("subPropertyOf")];
                                let subProperties = propertyValue.split(",");
                                elProperty.checked = subProperties.indexOf(propertyName) > -1;
                            } else if (elProperty.getAttribute("type") === "number") {
                                let fixedDecimals = elProperty.getAttribute("fixedDecimals");
                                elProperty.value = propertyValue.toFixed(fixedDecimals);
                            } else if (elProperty.getAttribute("type") === "checkbox") {
                                let inverse = elProperty.getAttribute("inverse") === "true";
                                elProperty.checked = inverse ? !propertyValue : propertyValue;
                            } else if (elProperty.nodeName === "TEXTAREA") {
                                elProperty.value = propertyValue;
                                setTextareaScrolling(elProperty);
                            } else if (elProperty.nodeName === "DT") { // dropdown
                                elProperty.value = propertyValue;
                                setDropdownText(elProperty);
                            } else {
                                elProperty.value = propertyValue;
                            }
                            
                            let propertyShowRules = showPropertyRules[propertyName];
                            if (propertyShowRules !== undefined) {
                                for (let propertyToShow in propertyShowRules) {
                                    let showIfThisPropertyValue = propertyShowRules[propertyToShow];
                                    let show = String(propertyValue) === String(showIfThisPropertyValue);
                                    let elPropertyToShow = elPropertyElements[propertyToShow];
                                    if (elPropertyToShow) {
                                        let nodeToShow = elPropertyToShow;
                                        if (elPropertyToShow.nodeName !== "DIV") {
                                            let parentNode = elPropertyToShow.parentNode;
                                            if (parentNode === undefined && elPropertyToShow instanceof Array) {
                                                parentNode = elPropertyToShow[0].parentNode;
                                            }
                                            if (parentNode !== undefined) {
                                                nodeToShow = parentNode;
                                            }
                                        }
                                        nodeToShow.style.display = show ? "table" : "none";
                                    }
                                }
                            }
                        }
                        
                        let elGrabbable = elPropertyElements["grabbable"];
                        let elTriggerable = elPropertyElements["triggerable"];
                        let elIgnoreIK = elPropertyElements["ignoreIK"];
                        elGrabbable.checked = elPropertyElements["dynamic"].checked;
                        elTriggerable.checked = false;
                        elIgnoreIK.checked = true;
                        let grabbablesSet = false;
                        let parsedUserData = {};
                        try {
                            parsedUserData = JSON.parse(properties.userData);
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
                        
                        if (properties.type === "Image") {
                            var imageLink = JSON.parse(properties.textures)["tex.picture"];
                            elPropertyElements["image"].value = imageLink;
                        } else if (properties.type === "Material") {
                            let elParentMaterialNameString = elPropertyElements["materialNameToReplace"];
                            let elParentMaterialNameNumber = elPropertyElements["submeshToReplace"];
                            let elParentMaterialNameCheckbox = elPropertyElements["selectSubmesh"];
                            if (properties.parentMaterialName.startsWith(MATERIAL_PREFIX_STRING)) {
                                elParentMaterialNameString.value = properties.parentMaterialName.replace(MATERIAL_PREFIX_STRING, "");
                                showParentMaterialNameBox(false, elParentMaterialNameNumber, elParentMaterialNameString);
                                elParentMaterialNameCheckbox.checked = false;
                            } else {
                                elParentMaterialNameNumber.value = parseInt(properties.parentMaterialName);
                                showParentMaterialNameBox(true, elParentMaterialNameNumber, elParentMaterialNameString);
                                elParentMaterialNameCheckbox.checked = true;
                            }
                        }
                        
                        let json = null;
                        try {
                            json = JSON.parse(properties.userData);
                        } catch (e) {
                            // normal text
                            deleteJSONEditor();
                            elPropertyElements["userData"].value = properties.userData;
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
                            materialJson = JSON.parse(properties.materialData);
                        } catch (e) {
                            // normal text
                            deleteJSONMaterialEditor();
                            elPropertyElements["materialData"].value = properties.materialData;
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
                        
                        if (properties.locked) {
                            disableProperties();
                            elPropertyElements["locked"].removeAttribute('disabled');
                        } else {
                            enableProperties();
                            disableSaveUserDataButton();
                            disableSaveMaterialDataButton()
                        }
                        
                        var activeElement = document.activeElement;
                        if (doSelectElement && typeof activeElement.select !== "undefined") {
                            activeElement.select();
                        }
                    }
                }
            });
        }
        
        // Server Script Status
        let elServerScript = elPropertyElements["serverScripts"];
        let elDiv = document.createElement('div');
        elDiv.setAttribute("class", "property");
        let elLabel = document.createElement('label');
        elLabel.setAttribute("for", "property-serverScripts-status");
        elLabel.innerText = "Server Script Status";
        let elServerScriptStatus = document.createElement('span');
        elServerScriptStatus.setAttribute("id", "property-serverScripts-status");
        elDiv.appendChild(elLabel);
        elDiv.appendChild(elServerScriptStatus);
        elServerScript.parentNode.appendChild(elDiv);
        
        // Server Script Error
        elDiv = document.createElement('div');
        elDiv.setAttribute("class", "property");
        let elServerScriptError = document.createElement('textarea');
        elServerScriptError.setAttribute("id", "property-serverScripts-error");
        elDiv.appendChild(elServerScriptError);
        elServerScript.parentNode.appendChild(elDiv);
        
        let elScript = elPropertyElements["script"];
        elScript.parentNode.setAttribute("class", "property url refresh");
        elServerScript.parentNode.setAttribute("class", "property url refresh");
            
        // User Data
        let elUserData = elPropertyElements["userData"];
        elDiv = elUserData.parentNode;
        let elStaticUserData = document.createElement('div');
        elStaticUserData.setAttribute("id", "property-userData-static");
        let elUserDataEditor = document.createElement('div');
        elUserDataEditor.setAttribute("id", "property-userData-editor");
        let elUserDataSaved = document.createElement('span');
        elUserDataSaved.setAttribute("id", "property-userData-saved");
        elUserDataSaved.innerText = "Saved!";
        elDiv.childNodes[2].appendChild(elUserDataSaved);
        elDiv.insertBefore(elStaticUserData, elUserData);
        elDiv.insertBefore(elUserDataEditor, elUserData);
        
        // Material Data
        let elMaterialData = elPropertyElements["materialData"];
        elDiv = elMaterialData.parentNode;
        let elStaticMaterialData = document.createElement('div');
        elStaticMaterialData.setAttribute("id", "property-materialData-static");
        let elMaterialDataEditor = document.createElement('div');
        elMaterialDataEditor.setAttribute("id", "property-materialData-editor");
        let elMaterialDataSaved = document.createElement('span');
        elMaterialDataSaved.setAttribute("id", "property-materialData-saved");
        elMaterialDataSaved.innerText = "Saved!";
        elDiv.childNodes[2].appendChild(elMaterialDataSaved);
        elDiv.insertBefore(elStaticMaterialData, elMaterialData);
        elDiv.insertBefore(elMaterialDataEditor, elMaterialData);
        
        // User Data Fields
        let elGrabbable = elPropertyElements["grabbable"];
        let elTriggerable = elPropertyElements["triggerable"];
        let elIgnoreIK = elPropertyElements["ignoreIK"];
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
        let elParentMaterialNameString = elPropertyElements["materialNameToReplace"];
        let elParentMaterialNameNumber = elPropertyElements["submeshToReplace"];
        let elParentMaterialNameCheckbox = elPropertyElements["selectSubmesh"];
        elParentMaterialNameString.addEventListener('change', function () { updateProperty("parentMaterialName", MATERIAL_PREFIX_STRING + this.value); });
        elParentMaterialNameNumber.addEventListener('change', function () { updateProperty("parentMaterialName", this.value); });
        elParentMaterialNameCheckbox.addEventListener('change', function () {
            if (this.checked) {
                updateProperty("parentMaterialName", elParentMaterialNameNumber.value);
                showParentMaterialNameBox(true, elParentMaterialNameNumber, elParentMaterialNameString);
            } else {
                updateProperty("parentMaterialName", MATERIAL_PREFIX_STRING + elParentMaterialNameString.value);
                showParentMaterialNameBox(false, elParentMaterialNameNumber, elParentMaterialNameString);
            }
        });
        
        elPropertyElements["image"].addEventListener('change', createImageURLUpdateFunction('textures'));
        
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
            
            let propertyName = elDropdowns[dropDownIndex].getAttribute("propertyName");
            elPropertyElements[propertyName] = dt;
            dt.addEventListener('change', createEmitTextPropertyUpdateFunction(propertyName));
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
