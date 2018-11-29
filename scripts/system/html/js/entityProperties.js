//  entityProperties.js
//
//  Created by Ryan Huffman on 13 Nov 2014
//  Modified by David Back on 19 Oct 2018
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

const DEGREES_TO_RADIANS = Math.PI / 180.0;

const NO_SELECTION = ",";

const PROPERTY_SPACE_MODE = {
    ALL: 0,
    LOCAL: 1,
    WORLD: 2
};

const GROUPS = [
    {
        id: "base",
        properties: [
            {
                label: NO_SELECTION,
                type: "icon",
                icons: ICON_FOR_TYPE,
                propertyID: "type",
                replaceID: "placeholder-property-type",
            },
            {
                label: "Name",
                type: "string",
                propertyID: "name",
                placeholder: "Name",
                replaceID: "placeholder-property-name",
            },
            {
                label: "ID",
                type: "string",
                propertyID: "id",
                placeholder: "ID",
                readOnly: true,
                replaceID: "placeholder-property-id",
            },
            {
                label: "Description",
                type: "string",
                propertyID: "description",
            },
            {
                label: "Parent",
                type: "string",
                propertyID: "parentID",
            },
            {
                label: "Parent Joint Index",
                type: "number",
                propertyID: "parentJointIndex",
            },
            {
                label: "",
                glyph: "&#xe006;",
                type: "bool",
                propertyID: "locked",
                replaceID: "placeholder-property-locked",
            },
            {
                label: "",
                glyph: "&#xe007;",
                type: "bool",
                propertyID: "visible",
                replaceID: "placeholder-property-visible",
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
                decimals: 4,
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
                decimals: 2,
                propertyID: "keyLight.intensity",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Light Horizontal Angle",
                type: "number",
                multiplier: DEGREES_TO_RADIANS,
                decimals: 2,
                unit: "deg",
                propertyID: "keyLight.direction.x",
                showPropertyRule: { "keyLightMode": "enabled" },
            },
            {
                label: "Light Vertical Angle",
                type: "number",
                multiplier: DEGREES_TO_RADIANS,
                decimals: 2,
                unit: "deg",
                propertyID: "keyLight.direction.y",
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
                label: "Skybox Source",
                type: "string",
                propertyID: "skybox.url",
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
                decimals: 2,
                propertyID: "ambientLight.ambientIntensity",
                showPropertyRule: { "ambientLightMode": "enabled" },
            },
            {
                label: "Ambient Source",
                type: "string",
                propertyID: "ambientLight.ambientURL",
                showPropertyRule: { "ambientLightMode": "enabled" },
            },
            {
                type: "buttons",
                buttons: [ { id: "copy", label: "Copy from Skybox", 
                             className: "black", onClick: copySkyboxURLToAmbientURL } ],
                propertyID: "copyURLToAmbient",
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
                decimals: 0,
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
                decimals: 0,
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
                decimals: 0,
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
                min: 0,
                max: 1,
                step: 0.01,
                decimals: 2,
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
                decimals: 0,
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
                decimals: 2,
                propertyID: "bloom.bloomIntensity",
                showPropertyRule: { "bloomMode": "enabled" },
            },
            {
                label: "Bloom Threshold",
                type: "number",
                min: 0,
                max: 1,
                step: 0.01,
                decimals: 2,
                propertyID: "bloom.bloomThreshold",
                showPropertyRule: { "bloomMode": "enabled" },
            },
            {
                label: "Bloom Size",
                type: "number",
                min: 0,
                max: 2,
                step: 0.01,
                decimals: 2,
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
                label: "Loop",
                type: "bool",
                propertyID: "animation.loop",
            },
            {
                label: "Allow Transition",
                type: "bool",
                propertyID: "animation.allowTranslation",
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
                placeholder: "URL",
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
                decimals: 1,
                propertyID: "intensity",
            },
            {
                label: "Fall-Off Radius",
                type: "number",
                min: 0,
                step: 0.1,
                decimals: 1,
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
                decimals: 2,
                propertyID: "exponent",
            },
            {
                label: "Spotlight Cut-Off",
                type: "number",
                step: 0.01,
                decimals: 2,
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
                vec2Type: "xy",
                min: 0,
                max: 1,
                step: 0.1,
                decimals: 4,
                subLabels: [ "x", "y" ],
                propertyID: "materialMappingPos",
            },
            {
                label: "Material Scale",
                type: "vec2",
                vec2Type: "wh",
                min: 0,
                step: 0.1,
                decimals: 4,
                subLabels: [ "width", "height" ],
                propertyID: "materialMappingScale",
            },
            {
                label: "Material Rotation",
                type: "number",
                step: 0.1,
                decimals: 2,
                unit: "deg",
                propertyID: "materialMappingRot",
            },
        ]
    },
    {
        id: "particles",
        addToGroup: "base",
        properties: [
            {
                label: "Emit",
                type: "bool",
                propertyID: "isEmitting",
            },
            {
                label: "Lifespan",
                type: "number",
                unit: "s",
                min: 0.01,
                max: 10,
                step: 0.01,
                decimals: 2,
                propertyID: "lifespan",
            },
            {
                label: "Max Particles",
                type: "number",
                min: 1,
                max: 10000,
                step: 1,
                propertyID: "maxParticles",
            },
            {
                label: "Texture",
                type: "texture",
                propertyID: "particleTextures",
                propertyName: "textures", // actual entity property name
            },
        ]
    },
    {
        id: "particles_emit",
        label: "EMIT",
        isMinor: true,
        properties: [
            {
                label: "Emit Rate",
                type: "number",
                min: 1,
                max: 1000,
                step: 1,
                propertyID: "emitRate",
            },
            {
                label: "Emit Speed",
                type: "number",
                min: 0,
                max: 5,
                step: 0.01,
                decimals: 2,
                propertyID: "emitSpeed",
            },
            {
                label: "Speed Spread",
                type: "number",
                min: 0,
                max: 5,
                step: 0.01,
                decimals: 2,
                propertyID: "speedSpread",
            },
            {
                label: "Emit Dimensions",
                type: "vec3",
                vec3Type: "xyz",
                min: 0,
                step: 0.01,
                round: 100,
                subLabels: [ "x", "y", "z" ],
                propertyID: "emitDimensions",
            },
            {
                label: "Emit Radius Start",
                type: "number",
                min: 0,
                max: 1,
                step: 0.01,
                decimals: 2,
                propertyID: "emitRadiusStart"
            },
            {
                label: "Emit Orientation",
                type: "vec3",
                vec3Type: "pyr",
                step: 0.01,
                round: 100,
                subLabels: [ "x", "y", "z" ],
                unit: "deg",
                propertyID: "emitOrientation",
            },
            {
                label: "Trails",
                type: "bool",
                propertyID: "emitterShouldTrail",
            },
        ]
    },
    {
        id: "particles_size",
        label: "SIZE",
        isMinor: true,
        properties: [
            {
                type: "triple",
                label: "Size",
                properties: [
                    {
                        label: "Start",
                        type: "number",
                        min: 0,
                        max: 4,
                        step: 0.01,
                        decimals: 2,
                        propertyID: "radiusStart",
                        fallbackProperty: "particleRadius",
                    },
                    {
                        label: "Middle",
                        type: "number",
                        min: 0,
                        max: 4,
                        step: 0.01,
                        decimals: 2,
                        propertyID: "particleRadius",
                    },
                    {
                        label: "Finish",
                        type: "number",
                        min: 0,
                        max: 4,
                        step: 0.01,
                        decimals: 2,
                        propertyID: "radiusFinish",
                        fallbackProperty: "particleRadius",
                    },
                ]
            },
            {
                label: "Size Spread",
                type: "number",
                min: 0,
                max: 4,
                step: 0.01,
                decimals: 2,
                propertyID: "radiusSpread",
            },
        ]
    },
    {
        id: "particles_color",
        label: "COLOR",
        isMinor: true,
        properties: [
            {
                type: "triple",
                label: "Color",
                properties: [
                    {
                        label: "Start",
                        type: "color",
                        propertyID: "colorStart",
                        fallbackProperty: "color",
                    },
                    {
                        label: "Middle",
                        type: "color",
                        propertyID: "particleColor",
                        propertyName: "color", // actual entity property name
                    },
                    {
                        label: "Finish",
                        type: "color",
                        propertyID: "colorFinish",
                        fallbackProperty: "color",
                    },
                ]
            },
            {
                label: "Color Spread",
                type: "color",
                propertyID: "colorSpread",
            },
        ]
    },
    {
        id: "particles_alpha",
        label: "ALPHA",
        isMinor: true,
        properties: [
            {
                type: "triple",
                label: "Alpha",
                properties: [
                    {
                        label: "Start",
                        type: "number",
                        min: 0,
                        max: 1,
                        step: 0.01,
                        decimals: 2,
                        propertyID: "alphaStart",
                        fallbackProperty: "alpha",
                    },
                    {
                        label: "Middle",
                        type: "number",
                        min: 0,
                        max: 1,
                        step: 0.01,
                        decimals: 2,
                        propertyID: "alpha",
                    },
                    {
                        label: "Finish",
                        type: "number",
                        min: 0,
                        max: 1,
                        step: 0.01,
                        decimals: 2,
                        propertyID: "alphaFinish",
                        fallbackProperty: "alpha",
                    },
                ]
            },
            {
                label: "Alpha Spread",
                type: "number",
                min: 0,
                max: 1,
                step: 0.01,
                decimals: 2,
                propertyID: "alphaSpread",
            },
        ]
    },
    {
        id: "particles_acceleration",
        label: "ACCELERATION",
        isMinor: true,
        properties: [
            {
                label: "Emit Acceleration",
                type: "vec3",
                vec3Type: "xyz",
                step: 0.01,
                round: 100,
                subLabels: [ "x", "y", "z" ],
                propertyID: "emitAcceleration",
            },
            {
                label: "Acceleration Spread",
                type: "vec3",
                vec3Type: "xyz",
                step: 0.01,
                round: 100,
                subLabels: [ "x", "y", "z" ],
                propertyID: "accelerationSpread",
            },
        ]
    },
    {
        id: "particles_spin",
        label: "SPIN",
        isMinor: true,
        properties: [
            {
                type: "triple",
                label: "Alpha",
                properties: [
                    {
                        label: "Start",
                        type: "number",
                        min: -360,
                        max: 360,
                        step: 1,
                        decimals: 0,
                        multiplier: DEGREES_TO_RADIANS,
                        unit: "deg",
                        propertyID: "spinStart",
                        fallbackProperty: "particleSpin",
                    },
                    {
                        label: "Middle",
                        type: "number",
                        min: -360,
                        max: 360,
                        step: 1,
                        decimals: 0,
                        multiplier: DEGREES_TO_RADIANS,
                        unit: "deg",
                        propertyID: "particleSpin",
                    },
                    {
                        label: "Finish",
                        type: "number",
                        min: -360,
                        max: 360,
                        step: 1,
                        decimals: 0,
                        multiplier: DEGREES_TO_RADIANS,
                        unit: "deg",
                        propertyID: "spinFinish",
                        fallbackProperty: "particleSpin",
                    },
                ]
            },
            {
                label: "Spin Spread",
                type: "number",
                min: 0,
                max: 360,
                step: 1,
                decimals: 0,
                multiplier: DEGREES_TO_RADIANS,
                unit: "deg",
                propertyID: "spinSpread",
            },
            {
                label: "Rotate with Entity",
                type: "bool",
                propertyID: "rotateWithEntity",
            },
        ]
    },
    {
        id: "particles_constraints",
        label: "CONSTRAINTS",
        isMinor: true,
        properties: [
            {
                type: "triple",
                label: "Horizontal Angle",
                properties: [
                    {
                        label: "Start",
                        type: "number",
                        min: -180,
                        max: 0,
                        step: 1,
                        decimals: 0,
                        multiplier: DEGREES_TO_RADIANS,
                        unit: "deg",
                        propertyID: "polarStart",
                    },
                    {
                        label: "Finish",
                        type: "number",
                        min: 0,
                        max: 180,
                        step: 1,
                        decimals: 0,
                        multiplier: DEGREES_TO_RADIANS,
                        unit: "deg",
                        propertyID: "polarFinish",
                    },
                ],
            },
            {
                type: "triple",
                label: "Vertical Angle",
                properties: [
                    {
                        label: "Start",
                        type: "number",
                        min: 0,
                        max: 180,
                        step: 1,
                        decimals: 0,
                        multiplier: DEGREES_TO_RADIANS,
                        unit: "deg",
                        propertyID: "azimuthStart",
                    },
                    {
                        label: "Finish",
                        type: "number",
                        min: 0,
                        max: 180,
                        step: 1,
                        decimals: 0,
                        multiplier: DEGREES_TO_RADIANS,
                        unit: "deg",
                        propertyID: "azimuthFinish",
                    },
                ]
            }
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
                decimals: 4,
                subLabels: [ "x", "y", "z" ],
                unit: "m",
                propertyID: "position",
                spaceMode: PROPERTY_SPACE_MODE.WORLD,
            },
            {
                label: "Local Position",
                type: "vec3",
                vec3Type: "xyz",
                decimals: 4,
                subLabels: [ "x", "y", "z" ],
                unit: "m",
                propertyID: "localPosition",
                spaceMode: PROPERTY_SPACE_MODE.LOCAL,
            },
            {
                label: "Rotation",
                type: "vec3",
                vec3Type: "pyr",
                step: 0.1,
                decimals: 4,
                subLabels: [ "x", "y", "z" ],
                unit: "deg",
                propertyID: "rotation",
                spaceMode: PROPERTY_SPACE_MODE.WORLD,
            },
            {
                label: "Local Rotation",
                type: "vec3",
                vec3Type: "pyr",
                step: 0.1,
                decimals: 4,
                subLabels: [ "x", "y", "z" ],
                unit: "deg",
                propertyID: "localRotation",
                spaceMode: PROPERTY_SPACE_MODE.LOCAL,
            },
            {
                label: "Dimensions",
                type: "vec3",
                vec3Type: "xyz",
                min: 0,
                step: 0.1,
                decimals: 4,
                subLabels: [ "x", "y", "z" ],
                unit: "m",
                propertyID: "dimensions",
                spaceMode: PROPERTY_SPACE_MODE.WORLD,
            },
            {
                label: "Local Dimensions",
                type: "vec3",
                vec3Type: "xyz",
                min: 0,
                step: 0.1,
                decimals: 4,
                subLabels: [ "x", "y", "z" ],
                unit: "m",
                propertyID: "localDimensions",
                spaceMode: PROPERTY_SPACE_MODE.LOCAL,
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
                vec3Type: "xyz",
                step: 0.1,
                decimals: 4,
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
        id: "behavior",
        label: "BEHAVIOR",
        properties: [
            {
                label: "Grabbable",
                type: "bool",
                propertyID: "grab.grabbable",
            },
            {
                label: "Cloneable",
                type: "bool",
                propertyID: "cloneable",
            },
            {
                label: "Clone Lifetime",
                type: "number",
                unit: "s",
                propertyID: "cloneLifetime",
                showPropertyRule: { "cloneable": "true" },
            },
            {
                label: "Clone Limit",
                type: "number",
                propertyID: "cloneLimit",
                showPropertyRule: { "cloneable": "true" },
            },
            {
                label: "Clone Dynamic",
                type: "bool",
                propertyID: "cloneDynamic",
                showPropertyRule: { "cloneable": "true" },
            },
            {
                label: "Clone Avatar Entity",
                type: "bool",
                propertyID: "cloneAvatarEntity",
                showPropertyRule: { "cloneable": "true" },
            },
            {
                label: "Triggerable",
                type: "bool",
                propertyID: "grab.triggerable",
            },
            {
                label: "Follow Controller",
                type: "bool",
                propertyID: "grab.grabFollowsController",
            },
            {
                label: "Cast shadows",
                type: "bool",
                propertyID: "canCastShadow",
            },
            {
                label: "Link",
                type: "string",
                propertyID: "href",
                placeholder: "URL",
            },
            {
                label: "Script",
                type: "string",
                buttons: [ { id: "reload", label: "F", className: "glyph", onClick: reloadScripts } ],
                propertyID: "script",
                placeholder: "URL",
            },
            {
                label: "Server Script",
                type: "string",
                buttons: [ { id: "reload", label: "F", className: "glyph", onClick: reloadServerScripts } ],
                propertyID: "serverScripts",
                placeholder: "URL",
            },
            {
                label: "Server Script Status",
                type: "placeholder",
                indentedLabel: true,
                propertyID: "serverScriptStatus",
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
        id: "collision",
        label: "COLLISION",
        properties: [
            {
                label: "Collides",
                type: "bool",
                inverse: true,
                propertyID: "collisionless",
            },
            {
                label: "Static Entities",
                type: "bool",
                propertyID: "collidesWithStatic",
                propertyName: "static", // actual subProperty name
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
            },
            {
                label: "Kinematic Entities",
                type: "bool",
                propertyID: "collidesWithKinematic",
                propertyName: "kinematic", // actual subProperty name
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
            },
            {
                label: "Dynamic Entities",
                type: "bool",
                propertyID: "collidesWithDynamic",
                propertyName: "dynamic", // actual subProperty name
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
            },
            {
                label: "My Avatar",
                type: "bool",
                propertyID: "collidesWithMyAvatar",
                propertyName: "myAvatar", // actual subProperty name
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
            },
            {
                label: "Other Avatars",
                type: "bool",
                propertyID: "collidesWithOtherAvatar",
                propertyName: "otherAvatar", // actual subProperty name
                subPropertyOf: "collidesWith",
                showPropertyRule: { "collisionless": "false" },
            },
            {
                label: "Collision Sound",
                type: "string",
                placeholder: "URL",
                propertyID: "collisionSoundURL",
                showPropertyRule: { "collisionless": "false" },
            },
            {
                label: "Dynamic",
                type: "bool",
                propertyID: "dynamic",
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
                step: 0.1,
                decimals: 4,
                subLabels: [ "x", "y", "z" ],
                unit: "m/s",
                propertyID: "localVelocity",
            },
            {
                label: "Linear Damping",
                type: "number",
                min: 0,
                max: 1,
                step: 0.01,
                decimals: 2,
                propertyID: "damping",
            },
            {
                label: "Angular Velocity",
                type: "vec3",
                vec3Type: "pyr",
                multiplier: DEGREES_TO_RADIANS,
                decimals: 4,
                subLabels: [ "x", "y", "z" ],
                unit: "deg/s",
                propertyID: "localAngularVelocity",
            },
            {
                label: "Angular Damping",
                type: "number",
                min: 0,
                max: 1,
                step: 0.01,
                decimals: 4,
                propertyID: "angularDamping",
            },
            {
                label: "Bounciness",
                type: "number",
                min: 0,
                max: 1,
                step: 0.01,
                decimals: 4,
                propertyID: "restitution",
            },
            {
                label: "Friction",
                type: "number",
                min: 0,
                max: 10,
                step: 0.1,
                decimals: 4,
                propertyID: "friction",
            },
            {
                label: "Density",
                type: "number",
                min: 100,
                max: 10000,
                step: 1,
                decimals: 4,
                propertyID: "density",
            },
            {
                label: "Gravity",
                type: "vec3",
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                step: 0.1,
                decimals: 4,
                unit: "m/s<sup>2</sup>",
                propertyID: "gravity",
            },
            {
                label: "Acceleration",
                type: "vec3",
                vec3Type: "xyz",
                subLabels: [ "x", "y", "z" ],
                decimals: 4,
                unit: "m/s<sup>2</sup>",
                propertyID: "acceleration",
            },
        ]
    },
];

const GROUPS_PER_TYPE = {
  None: [ 'base', 'spatial', 'behavior', 'collision', 'physics' ],
  Shape: [ 'base', 'shape', 'spatial', 'behavior', 'collision', 'physics' ],
  Text: [ 'base', 'text', 'spatial', 'behavior', 'collision', 'physics' ],
  Zone: [ 'base', 'zone', 'spatial', 'behavior', 'collision', 'physics' ],
  Model: [ 'base', 'model', 'spatial', 'behavior', 'collision', 'physics' ],
  Image: [ 'base', 'image', 'spatial', 'behavior', 'collision', 'physics' ],
  Web: [ 'base', 'web', 'spatial', 'behavior', 'collision', 'physics' ],
  Light: [ 'base', 'light', 'spatial', 'behavior', 'collision', 'physics' ],
  Material: [ 'base', 'material', 'spatial', 'behavior' ],
  ParticleEffect: [ 'base', 'particles', 'particles_emit', 'particles_size', 'particles_color', 'particles_alpha', 
                    'particles_acceleration', 'particles_spin', 'particles_constraints', 'spatial', 'behavior', 'physics' ],
  PolyLine: [ 'base', 'spatial', 'behavior', 'collision', 'physics' ],
  PolyVox: [ 'base', 'spatial', 'behavior', 'collision', 'physics' ],
  Multiple: [ 'base', 'spatial', 'behavior', 'collision', 'physics' ],
};

const EDITOR_TIMEOUT_DURATION = 1500;
const DEBOUNCE_TIMEOUT = 125;

const COLOR_MIN = 0;
const COLOR_MAX = 255;
const COLOR_STEP = 1;

const MATERIAL_PREFIX_STRING = "mat::";

const PENDING_SCRIPT_STATUS = "[ Fetching status ]";
const NOT_RUNNING_SCRIPT_STATUS = "Not running";
const ENTITY_SCRIPT_STATUS = {
    pending: "Pending",
    loading: "Loading",
    error_loading_script: "Error loading script", // eslint-disable-line camelcase
    error_running_script: "Error running script", // eslint-disable-line camelcase
    running: "Running",
    unloaded: "Unloaded"
};

const PROPERTY_NAME_DIVISION = {
    GROUP: 0,
    PROPERTY: 1,
    SUBPROPERTY: 2,
};

const VECTOR_ELEMENTS = {
    X_NUMBER: 0,
    Y_NUMBER: 1,
    Z_NUMBER: 2,
};

const COLOR_ELEMENTS = {
    COLOR_PICKER: 0,
    RED_NUMBER: 1,
    GREEN_NUMBER: 2,
    BLUE_NUMBER: 3,
};

const TEXTURE_ELEMENTS = {
    IMAGE: 0,
    TEXT_INPUT: 1,
};

const JSON_EDITOR_ROW_DIV_INDEX = 2;

var elGroups = {};
var properties = {};
var colorPickers = {};
var particlePropertyUpdates = {};
var selectedEntityProperties;
var lastEntityID = null;
var createAppTooltip = new CreateAppTooltip();
let currentSpaceMode = PROPERTY_SPACE_MODE.LOCAL;

function createElementFromHTML(htmlString) {
    let elTemplate = document.createElement('template');
    elTemplate.innerHTML = htmlString.trim();
    return elTemplate.content.firstChild;
}

/**
 * GENERAL PROPERTY/GROUP FUNCTIONS
 */

function getPropertyInputElement(propertyID) {
    let property = properties[propertyID];          
    switch (property.data.type) {
        case 'string':
        case 'bool':
        case 'dropdown':
        case 'textarea':
        case 'texture':
            return property.elInput;
        case 'number':
            return property.elNumber.elInput;
        case 'vec3': 
        case 'vec2':
            return { x: property.elNumberX.elInput, y: property.elNumberY.elInput, z: property.elNumberZ.elInput };
        case 'color':
            return { red: property.elNumberR.elInput, green: property.elNumberG.elInput, blue: property.elNumberB.elInput };
        case 'icon':
            return property.elLabel;
        default:
            return undefined;
    }
}

function enableChildren(el, selector) {
    let elSelectors = el.querySelectorAll(selector);
    for (let selectorIndex = 0; selectorIndex < elSelectors.length; ++selectorIndex) {
        elSelectors[selectorIndex].removeAttribute('disabled');
    }
}

function disableChildren(el, selector) {
    let elSelectors = el.querySelectorAll(selector);
    for (let selectorIndex = 0; selectorIndex < elSelectors.length; ++selectorIndex) {
        elSelectors[selectorIndex].setAttribute('disabled', 'disabled');
    }
}

function enableProperties() {
    enableChildren(document.getElementById("properties-list"),
                   "input, textarea, checkbox, .dropdown dl, .color-picker , .draggable-number.text");
    enableChildren(document, ".colpick");
    
    let elLocked = getPropertyInputElement("locked");
    if (elLocked.checked === false) {
        removeStaticUserData();
        removeStaticMaterialData();
    }
}

function disableProperties() {
    disableChildren(document.getElementById("properties-list"),
                    "input, textarea, checkbox, .dropdown dl, .color-picker, .draggable-number.text");
    disableChildren(document, ".colpick");
    for (let pickKey in colorPickers) {
        colorPickers[pickKey].colpickHide();
    }
    
    let elLocked = getPropertyInputElement("locked");
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
    let elProperty = properties[propertyID].elContainer;
    elProperty.style.display = show ? "" : "none";
}

function resetProperties() {
    for (let propertyID in properties) { 
        let property = properties[propertyID];      
        let propertyData = property.data;
        
        switch (propertyData.type) {
            case 'string': {
                property.elInput.value = "";
                break;
            }
            case 'bool': {
                property.elInput.checked = false;
                break;
            }
            case 'number': {
                if (propertyData.defaultValue !== undefined) {
                    property.elNumber.setValue(propertyData.defaultValue);
                } else { 
                    property.elNumber.setValue("");
                }
                break;
            }
            case 'vec3': 
            case 'vec2': {
                property.elNumberX.setValue("");
                property.elNumberY.setValue("");
                if (property.elNumberZ !== undefined) {
                    property.elNumberZ.setValue("");
                }
                break;
            }
            case 'color': {
                property.elColorPicker.style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                property.elNumberR.setValue("");
                property.elNumberG.setValue("");
                property.elNumberB.setValue("");
                break;
            }
            case 'dropdown': {
                property.elInput.value = "";
                setDropdownText(property.elInput);
                break;
            }
            case 'textarea': {
                property.elInput.value = "";
                setTextareaScrolling(property.elInput);
                break;
            }
            case 'icon': {
                property.elSpan.style.display = "none";
                break;
            }
            case 'texture': {
                property.elInput.value = "";
                property.elInput.imageLoad(property.elInput.value);
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
    
    let elServerScriptError = document.getElementById("property-serverScripts-error");
    let elServerScriptStatus = document.getElementById("property-serverScripts-status");
    elServerScriptError.parentElement.style.display = "none";
    elServerScriptStatus.innerText = NOT_RUNNING_SCRIPT_STATUS;
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

function getPropertyValue(originalPropertyName) {
    // if this is a compound property name (i.e. animation.running) 
    // then split it by . up to 3 times to find property value
    let propertyValue;
    let splitPropertyName = originalPropertyName.split('.');
    if (splitPropertyName.length > 1) {
        let propertyGroupName = splitPropertyName[PROPERTY_NAME_DIVISION.GROUP];
        let propertyName = splitPropertyName[PROPERTY_NAME_DIVISION.PROPERTY];
        let groupProperties = selectedEntityProperties[propertyGroupName];
        if (groupProperties === undefined || groupProperties[propertyName] === undefined) {
            return undefined;
        }
        if (splitPropertyName.length === PROPERTY_NAME_DIVISION.SUBPROPERTY + 1) { 
            let subPropertyName = splitPropertyName[PROPERTY_NAME_DIVISION.SUBPROPERTY];
            propertyValue = groupProperties[propertyName][subPropertyName];
        } else {
            propertyValue = groupProperties[propertyName];
        }
    } else {
        propertyValue = selectedEntityProperties[originalPropertyName];
    }
    return propertyValue;
}

function updateVisibleSpaceModeProperties() {
    for (let propertyID in properties) {
        if (properties.hasOwnProperty(propertyID)) {
            let property = properties[propertyID];
            let propertySpaceMode = property.spaceMode;
            let elProperty = properties[propertyID].elContainer;
            if (propertySpaceMode !== PROPERTY_SPACE_MODE.ALL && propertySpaceMode !== currentSpaceMode) {
                elProperty.classList.add('spacemode-hidden');
            } else {
                elProperty.classList.remove('spacemode-hidden');
            }
        }
    }
}


/**
 * PROPERTY UPDATE FUNCTIONS
 */

function updateProperty(originalPropertyName, propertyValue, isParticleProperty, blockUpdateCallback) {
    let propertyUpdate = {};
    // if this is a compound property name (i.e. animation.running) then split it by . up to 3 times
    let splitPropertyName = originalPropertyName.split('.');
    if (splitPropertyName.length > 1) {
        let propertyGroupName = splitPropertyName[PROPERTY_NAME_DIVISION.GROUP];
        let propertyName = splitPropertyName[PROPERTY_NAME_DIVISION.PROPERTY];
        propertyUpdate[propertyGroupName] = {};
        if (splitPropertyName.length === PROPERTY_NAME_DIVISION.SUBPROPERTY + 1) { 
            let subPropertyName = splitPropertyName[PROPERTY_NAME_DIVISION.SUBPROPERTY];
            propertyUpdate[propertyGroupName][propertyName] = {};
            propertyUpdate[propertyGroupName][propertyName][subPropertyName] = propertyValue;
        } else {
            propertyUpdate[propertyGroupName][propertyName] = propertyValue;
        }
    } else {
        propertyUpdate[originalPropertyName] = propertyValue;
    }
    // queue up particle property changes with the debounced sync to avoid  
    // causing particle emitting to reset excessively with each value change
    if (isParticleProperty) {
        Object.keys(propertyUpdate).forEach(function (propertyUpdateKey) {
            particlePropertyUpdates[propertyUpdateKey] = propertyUpdate[propertyUpdateKey];
        });
        particleSyncDebounce();
    } else {
        updateProperties(propertyUpdate, blockUpdateCallback);
    }
}

var particleSyncDebounce = _.debounce(function () {
    updateProperties(particlePropertyUpdates);
    particlePropertyUpdates = {};
}, DEBOUNCE_TIMEOUT);

function updateProperties(propertiesToUpdate, blockUpdateCallback) {
    if (blockUpdateCallback === undefined) {
        blockUpdateCallback = false;
    }
    EventBridge.emitWebEvent(JSON.stringify({
        id: lastEntityID,
        type: "update",
        properties: propertiesToUpdate,
        blockUpdateCallback: blockUpdateCallback
    }));
}

function createEmitTextPropertyUpdateFunction(property) {
    return function() {
        updateProperty(property.name, this.value, property.isParticleProperty);
    };
}

function createEmitCheckedPropertyUpdateFunction(property) {
    return function() {
        updateProperty(property.name, property.data.inverse ? !this.checked : this.checked, property.isParticleProperty);
    };
}

function createDragStartFunction(property) {
    return function() {
        property.dragging = true;
    };
}

function createDragEndFunction(property) {
    return function() {
        property.dragging = false;
        EventBridge.emitWebEvent(JSON.stringify({
            type: "updateProperties"
        }));
    };
}

function createEmitNumberPropertyUpdateFunction(property) {
    return function() {
        let multiplier = property.data.multiplier;
        if (multiplier === undefined) {
            multiplier = 1;
        }
        let value = parseFloat(this.value) * multiplier;
        updateProperty(property.name, value, property.isParticleProperty, property.dragging);
    };
}

function createEmitVec2PropertyUpdateFunction(property) {
    return function () {
        let multiplier = property.data.multiplier;
        if (multiplier === undefined) {
            multiplier = 1;
        }
        let newValue = {
            x: property.elNumberX.elInput.value * multiplier,
            y: property.elNumberY.elInput.value * multiplier
        };
        updateProperty(property.name, newValue, property.isParticleProperty, property.dragging);
    };
}

function createEmitVec3PropertyUpdateFunction(property) {
    return function() {
        let multiplier = property.data.multiplier;
        if (multiplier === undefined) {
            multiplier = 1;
        }
        let newValue = {
            x: property.elNumberX.elInput.value * multiplier,
            y: property.elNumberY.elInput.value * multiplier,
            z: property.elNumberZ.elInput.value * multiplier
        };
        updateProperty(property.name, newValue, property.isParticleProperty, property.dragging);
    };
}

function createEmitColorPropertyUpdateFunction(property) {
    return function() {
        emitColorPropertyUpdate(property.name, property.elNumberR.elInput.value, property.elNumberG.elInput.value,
                                property.elNumberB.elInput.value, property.isParticleProperty);
    };
}

function emitColorPropertyUpdate(propertyName, red, green, blue, isParticleProperty) {
    let newValue = {
        red: red,
        green: green,
        blue: blue
    };
    updateProperty(propertyName, newValue, isParticleProperty);
}

function updateCheckedSubProperty(propertyName, propertyValue, subPropertyElement, subPropertyString, isParticleProperty) {
    if (subPropertyElement.checked) {
        if (propertyValue.indexOf(subPropertyString)) {
            propertyValue += subPropertyString + ',';
        }
    } else {
        // We've unchecked, so remove
        propertyValue = propertyValue.replace(subPropertyString + ",", "");
    }
    updateProperty(propertyName, propertyValue, isParticleProperty);
}

function createImageURLUpdateFunction(property) {
    return function () {
        let newTextures = JSON.stringify({ "tex.picture": this.value });
        updateProperty(property.name, newTextures, property.isParticleProperty);
    };
}


/**
 * PROPERTY ELEMENT CREATION FUNCTIONS
 */

function createStringProperty(property, elProperty) {    
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.className = "text";
    
    let elInput = createElementFromHTML(`
        <input id="${elementID}"
               type="text"
               ${propertyData.placeholder ? 'placeholder="' + propertyData.placeholder + '"' : ''}
               ${propertyData.readOnly ? 'readonly' : ''}></input>
        `)

    
    elInput.addEventListener('change', createEmitTextPropertyUpdateFunction(property));
    
    elProperty.appendChild(elInput);
    
    if (propertyData.buttons !== undefined) {
        addButtons(elProperty, elementID, propertyData.buttons, false);
    }
    
    return elInput;
}

function createBoolProperty(property, elProperty) {   
    let propertyName = property.name;
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.className = "checkbox";
                        
    if (propertyData.glyph !== undefined) {
        let elSpan = document.createElement('span');
        elSpan.innerHTML = propertyData.glyph;
        elSpan.className = 'icon';
        elProperty.appendChild(elSpan);
    }
    
    let elInput = document.createElement('input');
    elInput.setAttribute("id", elementID);
    elInput.setAttribute("type", "checkbox");
    
    elProperty.appendChild(elInput);
    elProperty.appendChild(createElementFromHTML(`<label for=${elementID}>&nbsp;</label>`));
    
    let subPropertyOf = propertyData.subPropertyOf;
    if (subPropertyOf !== undefined) {
        elInput.addEventListener('change', function() {
            updateCheckedSubProperty(subPropertyOf, selectedEntityProperties[subPropertyOf], 
                                     elInput, propertyName, property.isParticleProperty);
        });
    } else {
        elInput.addEventListener('change', createEmitCheckedPropertyUpdateFunction(property));
    }
    
    return elInput;
}

function createNumberProperty(property, elProperty) { 
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.className = "draggable-number";

    let dragStartFunction = createDragStartFunction(property);
    let dragEndFunction = createDragEndFunction(property);
    let elDraggableNumber = new DraggableNumber(propertyData.min, propertyData.max, propertyData.step,
                                                propertyData.decimals, dragStartFunction, dragEndFunction);
    
    let defaultValue = propertyData.defaultValue;   
    if (defaultValue !== undefined) {   
        elDraggableNumber.elInput.value = defaultValue; 
    }

    let valueChangeFunction = createEmitNumberPropertyUpdateFunction(property);
    elDraggableNumber.setValueChangeFunction(valueChangeFunction);
    
    elDraggableNumber.elInput.setAttribute("id", elementID);
    elProperty.appendChild(elDraggableNumber.elDiv);

    if (propertyData.buttons !== undefined) {
        addButtons(elDraggableNumber.elText, elementID, propertyData.buttons, false);
    }
    
    return elDraggableNumber;
}

function createVec3Property(property, elProperty) {
    let propertyData = property.data;

    elProperty.className = propertyData.vec3Type + " fstuple";
    
    let elNumberX = createTupleNumberInput(property, propertyData.subLabels[VECTOR_ELEMENTS.X_NUMBER]);
    let elNumberY = createTupleNumberInput(property, propertyData.subLabels[VECTOR_ELEMENTS.Y_NUMBER]);
    let elNumberZ = createTupleNumberInput(property, propertyData.subLabels[VECTOR_ELEMENTS.Z_NUMBER]);
    elProperty.appendChild(elNumberX.elDiv);
    elProperty.appendChild(elNumberY.elDiv);
    elProperty.appendChild(elNumberZ.elDiv);
    
    let valueChangeFunction = createEmitVec3PropertyUpdateFunction(property);
    elNumberX.setValueChangeFunction(valueChangeFunction);
    elNumberY.setValueChangeFunction(valueChangeFunction);
    elNumberZ.setValueChangeFunction(valueChangeFunction);
    
    let elResult = [];
    elResult[VECTOR_ELEMENTS.X_NUMBER] = elNumberX;
    elResult[VECTOR_ELEMENTS.Y_NUMBER] = elNumberY;
    elResult[VECTOR_ELEMENTS.Z_NUMBER] = elNumberZ;
    return elResult;
}

function createVec2Property(property, elProperty) {  
    let propertyData = property.data;
    
    elProperty.className = propertyData.vec2Type + " fstuple";
                        
    let elTuple = document.createElement('div');
    elTuple.className = "tuple";
    
    elProperty.appendChild(elTuple);
    
    let elNumberX = createTupleNumberInput(property, propertyData.subLabels[VECTOR_ELEMENTS.X_NUMBER]);
    let elNumberY = createTupleNumberInput(property, propertyData.subLabels[VECTOR_ELEMENTS.Y_NUMBER]);
    elProperty.appendChild(elNumberX.elDiv);
    elProperty.appendChild(elNumberY.elDiv);
    
    let valueChangeFunction = createEmitVec2PropertyUpdateFunction(property);
    elNumberX.setValueChangeFunction(valueChangeFunction);
    elNumberY.setValueChangeFunction(valueChangeFunction);
    
    let elResult = [];
    elResult[VECTOR_ELEMENTS.X_NUMBER] = elNumberX;
    elResult[VECTOR_ELEMENTS.Y_NUMBER] = elNumberY;
    return elResult;
}

function createColorProperty(property, elProperty) {
    let propertyName = property.name;
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.className = "rgb fstuple";
    
    let elColorPicker = document.createElement('div');
    elColorPicker.className = "color-picker";
    elColorPicker.setAttribute("id", elementID);
    
    let elTuple = document.createElement('div');
    elTuple.className = "tuple";
    
    elProperty.appendChild(elColorPicker);
    elProperty.appendChild(elTuple);
    
    if (propertyData.min === undefined) {
        propertyData.min = COLOR_MIN;
    }
    if (propertyData.max === undefined) {
        propertyData.max = COLOR_MAX;
    }
    if (propertyData.step === undefined) {
        propertyData.step = COLOR_STEP;
    }
    
    let elNumberR = createTupleNumberInput(property, "red");
    let elNumberG = createTupleNumberInput(property, "green");
    let elNumberB = createTupleNumberInput(property, "blue");
    elTuple.appendChild(elNumberR.elDiv);
    elTuple.appendChild(elNumberG.elDiv);
    elTuple.appendChild(elNumberB.elDiv);
    
    let valueChangeFunction = createEmitColorPropertyUpdateFunction(property);
    elNumberR.setValueChangeFunction(valueChangeFunction);
    elNumberG.setValueChangeFunction(valueChangeFunction);
    elNumberB.setValueChangeFunction(valueChangeFunction);
    
    let colorPickerID = "#" + elementID;
    colorPickers[colorPickerID] = $(colorPickerID).colpick({
        colorScheme: 'dark',
        layout: 'rgbhex',
        color: '000000',
        submit: false, // We don't want to have a submission button
        onShow: function(colpick) {
            $(colorPickerID).attr('active', 'true');
            // The original color preview within the picker needs to be updated on show because
            // prior to the picker being shown we don't have access to the selections' starting color.
            colorPickers[colorPickerID].colpickSetColor({
                "r": elNumberR.elInput.value,
                "g": elNumberG.elInput.value,
                "b": elNumberB.elInput.value
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
    
    let elResult = [];
    elResult[COLOR_ELEMENTS.COLOR_PICKER] = elColorPicker;
    elResult[COLOR_ELEMENTS.RED_NUMBER] = elNumberR;
    elResult[COLOR_ELEMENTS.GREEN_NUMBER] = elNumberG;
    elResult[COLOR_ELEMENTS.BLUE_NUMBER] = elNumberB;
    return elResult;
}

function createDropdownProperty(property, propertyID, elProperty) { 
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.className = "dropdown";
                        
    let elInput = document.createElement('select');
    elInput.setAttribute("id", elementID);
    elInput.setAttribute("propertyID", propertyID);
    
    for (let optionKey in propertyData.options) {
        let option = document.createElement('option');
        option.value = optionKey;
        option.text = propertyData.options[optionKey];
        elInput.add(option);
    }
    
    elInput.addEventListener('change', createEmitTextPropertyUpdateFunction(property));
    
    elProperty.appendChild(elInput);
    
    return elInput;
}

function createTextareaProperty(property, elProperty) {   
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.className = "textarea";
    
    let elInput = document.createElement('textarea');
    elInput.setAttribute("id", elementID);
    if (propertyData.readOnly) {
        elInput.readOnly = true;
    }                   
    
    elInput.addEventListener('change', createEmitTextPropertyUpdateFunction(property));
    
    elProperty.appendChild(elInput);
                        
    if (propertyData.buttons !== undefined) {
        addButtons(elProperty, elementID, propertyData.buttons, true);
    }
    
    return elInput;
}

function createIconProperty(property, elProperty) { 
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.className = "value";
    
    let elSpan = document.createElement('span');
    elSpan.setAttribute("id", elementID + "-icon");
    elSpan.className = 'icon';

    elProperty.appendChild(elSpan);
    
    return elSpan;
}

function createTextureProperty(property, elProperty) { 
    let elementID = property.elementID;
    
    elProperty.className = "texture";
    
    let elDiv = document.createElement("div");
    let elImage = document.createElement("img");
    elDiv.className = "texture-image no-texture";
    elDiv.appendChild(elImage);
    
    let elInput = document.createElement('input');
    elInput.setAttribute("id", elementID);
    elInput.setAttribute("type", "text"); 
    
    let imageLoad = _.debounce(function (url) {
        if (url.slice(0, 5).toLowerCase() === "atp:/") {
            elImage.src = "";
            elImage.style.display = "none";
            elDiv.classList.remove("with-texture");
            elDiv.classList.remove("no-texture");
            elDiv.classList.add("no-preview");
        } else if (url.length > 0) {
            elDiv.classList.remove("no-texture");
            elDiv.classList.remove("no-preview");
            elDiv.classList.add("with-texture");
            elImage.src = url;
            elImage.style.display = "block";
        } else {
            elImage.src = "";
            elImage.style.display = "none";
            elDiv.classList.remove("with-texture");
            elDiv.classList.remove("no-preview");
            elDiv.classList.add("no-texture");
        }
    }, DEBOUNCE_TIMEOUT * 2);
    elInput.imageLoad = imageLoad;
    elInput.oninput = function (event) {
        // Add throttle
        let url = event.target.value;
        imageLoad(url);
        updateProperty(property.name, url, property.isParticleProperty)
    };
    elInput.onchange = elInput.oninput;

    elProperty.appendChild(elInput);
    elProperty.appendChild(elDiv);
   
    let elResult = [];
    elResult[TEXTURE_ELEMENTS.IMAGE] = elImage;
    elResult[TEXTURE_ELEMENTS.TEXT_INPUT] = elInput;
    return elResult;
}

function createButtonsProperty(property, elProperty, elLabel) {
    let elementID = property.elementID;
    let propertyData = property.data;
    
    elProperty.className = "text";

    if (propertyData.buttons !== undefined) {
        addButtons(elProperty, elementID, propertyData.buttons, false);
    }
    
    return elProperty;
}

function createTupleNumberInput(property, subLabel) {
    let propertyElementID = property.elementID;
    let propertyData = property.data;
    let elementID = propertyElementID + "-" + subLabel.toLowerCase();
    
    let elLabel = document.createElement('label');
    elLabel.className = "sublabel " + subLabel;
    elLabel.innerText = subLabel[0].toUpperCase() + subLabel.slice(1);
    elLabel.setAttribute("for", elementID);
    elLabel.style.visibility = "visible";
    
    let dragStartFunction = createDragStartFunction(property);
    let dragEndFunction = createDragEndFunction(property);
    let elDraggableNumber = new DraggableNumber(propertyData.min, propertyData.max, propertyData.step, 
                                                propertyData.decimals, dragStartFunction, dragEndFunction); 
    elDraggableNumber.elInput.setAttribute("id", elementID);
    elDraggableNumber.elDiv.className += " fstuple";
    elDraggableNumber.elText.insertBefore(elLabel, elDraggableNumber.elLeftArrow);
    
    return elDraggableNumber;
}

function addButtons(elProperty, propertyID, buttons, newRow) {
    let elDiv = document.createElement('div');
    elDiv.className = "row";
    
    buttons.forEach(function(button) {
        let elButton = document.createElement('input');
        elButton.className = button.className;
        elButton.setAttribute("type", "button");
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

function createProperty(propertyData, propertyElementID, propertyName, propertyID, elProperty) {
    let property = { 
        data: propertyData, 
        elementID: propertyElementID, 
        name: propertyName,
        elProperty: elProperty,
    };
    let propertyType = propertyData.type;

    switch (propertyType) {
        case 'string': {
            property.elInput = createStringProperty(property, elProperty);
            break;
        }
        case 'bool': {
            property.elInput = createBoolProperty(property, elProperty);
            break;
        }
        case 'number': {
            property.elNumber = createNumberProperty(property, elProperty);
            break;
        }
        case 'vec3': {
            let elVec3 = createVec3Property(property, elProperty);  
            property.elNumberX = elVec3[VECTOR_ELEMENTS.X_NUMBER];
            property.elNumberY = elVec3[VECTOR_ELEMENTS.Y_NUMBER];
            property.elNumberZ = elVec3[VECTOR_ELEMENTS.Z_NUMBER];
            break;
        }
        case 'vec2': {
            let elVec2 = createVec2Property(property, elProperty);  
            property.elNumberX = elVec2[VECTOR_ELEMENTS.X_NUMBER];
            property.elNumberY = elVec2[VECTOR_ELEMENTS.Y_NUMBER];
            break;
        }
        case 'color': {
            let elColor = createColorProperty(property, elProperty);  
            property.elColorPicker = elColor[COLOR_ELEMENTS.COLOR_PICKER];
            property.elNumberR = elColor[COLOR_ELEMENTS.RED_NUMBER];
            property.elNumberG = elColor[COLOR_ELEMENTS.GREEN_NUMBER];
            property.elNumberB = elColor[COLOR_ELEMENTS.BLUE_NUMBER]; 
            break;
        }
        case 'dropdown': {
            property.elInput = createDropdownProperty(property, propertyID, elProperty);
            break;
        }
        case 'textarea': {
            property.elInput = createTextareaProperty(property, elProperty);
            break;
        }
        case 'icon': {
            property.elSpan = createIconProperty(property, elProperty);
            break;
        }
        case 'texture': {
            let elTexture = createTextureProperty(property, elProperty);
            property.elImage = elTexture[TEXTURE_ELEMENTS.IMAGE];
            property.elInput = elTexture[TEXTURE_ELEMENTS.TEXT_INPUT];
            break;
        }
        case 'buttons': {
            property.elProperty = createButtonsProperty(property, elProperty);
            break;
        }
        case 'placeholder':
        case 'sub-header': {
            break;
        }
        default: {
            console.log("EntityProperties - Unknown property type " + 
                        propertyType + " set to property " + propertyID);
            break;
        }
    }

    return property;
}


/**
 * BUTTON CALLBACKS
 */

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
    let skyboxURL = getPropertyInputElement("skybox.url").value;
    getPropertyInputElement("ambientLight.ambientURL").value = skyboxURL;
    updateProperty("ambientLight.ambientURL", skyboxURL, false);
}


/**
 * USER DATA FUNCTIONS
 */

function clearUserData() {
    let elUserData = getPropertyInputElement("userData");
    deleteJSONEditor();
    elUserData.value = "";
    showUserDataTextArea();
    showNewJSONEditorButton();
    hideSaveUserDataButton();
    updateProperty('userData', elUserData.value, false);
}

function newJSONEditor() {
    deleteJSONEditor();
    createJSONEditor();
    let data = {};
    setEditorJSON(data);
    hideUserDataTextArea();
    hideNewJSONEditorButton();
    showSaveUserDataButton();
}

function saveUserData() {
    saveJSONUserData(true);
}

function setUserDataFromEditor(noUpdate) {
    let json = null;
    try {
        json = editor.get();
    } catch (e) {
        alert('Invalid JSON code - look for red X in your code ', +e);
    }
    if (json === null) {
        return;
    } else {
        let text = editor.getText();
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
            updateProperty('userData', text, false);
        }
    }
}

function multiDataUpdater(groupName, updateKeyPair, userDataElement, defaults, removeKeys) {
    let propertyUpdate = {};
    let parsedData = {};
    let keysToBeRemoved = removeKeys ? removeKeys : [];
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
    let keys = Object.keys(updateKeyPair);
    keys.forEach(function (key) {
        if (updateKeyPair[key] !== null && updateKeyPair[key] !== "null") {
            if (updateKeyPair[key] instanceof Element) {
                if (updateKeyPair[key].type === "checkbox") {
                    parsedData[groupName][key] = updateKeyPair[key].checked;
                } else {
                    let val = isNaN(updateKeyPair[key].value) ? updateKeyPair[key].value : parseInt(updateKeyPair[key].value);
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

    updateProperties(propertyUpdate, false);
}

var editor = null;

function createJSONEditor() {
    let container = document.getElementById("property-userData-editor");
    let options = {
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
            let currentJSONString = editor.getText();

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


/**
 * MATERIAL DATA FUNCTIONS
 */

function clearMaterialData() {
    let elMaterialData = getPropertyInputElement("materialData");
    deleteJSONMaterialEditor();
    elMaterialData.value = "";
    showMaterialDataTextArea();
    showNewJSONMaterialEditorButton();
    hideSaveMaterialDataButton();
    updateProperty('materialData', elMaterialData.value, false);
}

function newJSONMaterialEditor() {
    deleteJSONMaterialEditor();
    createJSONMaterialEditor();
    let data = {};
    setMaterialEditorJSON(data);
    hideMaterialDataTextArea();
    hideNewJSONMaterialEditorButton();
    showSaveMaterialDataButton();
}

function saveMaterialData() {
    saveJSONMaterialData(true);
}

function setMaterialDataFromEditor(noUpdate) {
    let json = null;
    try {
        json = materialEditor.get();
    } catch (e) {
        alert('Invalid JSON code - look for red X in your code ', +e);
    }
    if (json === null) {
        return;
    } else {
        let text = materialEditor.getText();
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
            updateProperty('materialData', text, false);
        }
    }
}

var materialEditor = null;

function createJSONMaterialEditor() {
    let container = document.getElementById("property-materialData-editor");
    let options = {
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
            let currentJSONString = materialEditor.getText();

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
    let inputs = $('input');
    let i;
    for (i = 0; i < inputs.length; ++i) {
        let input = inputs[i];
        let field = $(input);
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


/**
 * DROPDOWN FUNCTIONS
 */

function setDropdownText(dropdown) {
    let lis = dropdown.parentNode.getElementsByTagName("li");
    let text = "";
    for (let i = 0; i < lis.length; ++i) {
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


/**
 * TEXTAREA / PARENT MATERIAL NAME FUNCTIONS
 */

function setTextareaScrolling(element) {
    let isScrolling = element.scrollHeight > element.offsetHeight;
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

        let templatePropertyRow = document.getElementById('property-row');
        
        GROUPS.forEach(function(group) {
            let elGroup;
            if (group.addToGroup !== undefined) {
                let fieldset = document.getElementById("properties-" + group.addToGroup);
                elGroup = document.createElement('div');
                fieldset.appendChild(elGroup);
            } else {
                elGroup = document.createElement('div');
                elGroup.className = 'section ' + (group.isMinor ? "minor" : "major");
                elGroup.setAttribute("id", "properties-" + group.id);
                elPropertiesList.appendChild(elGroup);
            }       

            if (group.label !== undefined) {
                let elLegend = document.createElement('div');
                elLegend.className = "section-header";

                elLegend.appendChild(createElementFromHTML(`<div class="label">${group.label}</div>`));

                let elSpan = document.createElement('span');
                elSpan.className = "collapse-icon";
                elSpan.innerText = "M";
                elLegend.appendChild(elSpan);
                elGroup.appendChild(elLegend);
            }
                
            group.properties.forEach(function(propertyData) {
                let propertyType = propertyData.type;
                let propertyID = propertyData.propertyID;               
                let propertyName = propertyData.propertyName !== undefined ? propertyData.propertyName : propertyID;
                let propertySpaceMode = propertyData.spaceMode !== undefined ? propertyData.spaceMode : PROPERTY_SPACE_MODE.ALL;
                let propertyElementID = "property-" + propertyID;
                propertyElementID = propertyElementID.replace('.', '-');
                
                let elContainer, elLabel;
                
                if (propertyData.replaceID === undefined) {
                    // Create subheader, or create new property and append it.
                    if (propertyType === "sub-header") {
                        elContainer = createElementFromHTML(
                            `<div class="sub-section-header legend">${propertyData.label}</div>`);
                    } else {
                        elContainer = document.createElement('div');
                        elContainer.setAttribute("id", "div-" + propertyElementID);
                        elContainer.className = 'property container';
                    }

                    if (group.twoColumn && propertyData.column !== undefined && propertyData.column !== -1) {
                        let columnName = group.id + "column" + propertyData.column;
                        let elColumn = document.getElementById(columnName);
                        if (!elColumn) {
                            let columnDivName = group.id + "columnDiv";
                            let elColumnDiv = document.getElementById(columnDivName);
                            if (!elColumnDiv) {
                                elColumnDiv = document.createElement('div');
                                elColumnDiv.className = "two-column";
                                elColumnDiv.setAttribute("id", group.id + "columnDiv");
                                elGroup.appendChild(elColumnDiv);
                            }
                            elColumn = document.createElement('fieldset');
                            elColumn.className = "column";
                            elColumn.setAttribute("id", columnName);
                            elColumnDiv.appendChild(elColumn);
                        }
                        elColumn.appendChild(elContainer);
                    } else {
                        elGroup.appendChild(elContainer);
                    }

                    elLabel = document.createElement('label');
                    elLabel.setAttribute("for", propertyElementID);
                    if (propertyData.indentedLabel || propertyData.showPropertyRule !== undefined) {
                        let elSpan = document.createElement('span');
                        elSpan.className = 'indented';
                        elSpan.innerText = propertyData.label !== undefined ? propertyData.label : "";
                        elLabel.appendChild(elSpan);
                    } else {
                        elLabel.innerText = propertyData.label !== undefined ? propertyData.label : "";
                    }
                    elContainer.appendChild(elLabel);
                } else {
                    elContainer = document.getElementById(propertyData.replaceID);
                }

                if (elLabel) {
                    createAppTooltip.registerTooltipElement(elLabel, propertyID);
                }

                let elProperty = createElementFromHTML('<div style="width: 100%;"></div>');
                elContainer.appendChild(elProperty);

                if (propertyType === 'triple') {
                    elProperty.className = 'flex-row';
                    for (let i = 0; i < propertyData.properties.length; ++i) {
                        let innerPropertyData = propertyData.properties[i];

                        let elWrapper = createElementFromHTML('<div class="flex-column flex-center triple-item"><div></div></div>');

                        let propertyID = innerPropertyData.propertyID;               
                        let propertyName = innerPropertyData.propertyName !== undefined ? innerPropertyData.propertyName : propertyID;
                        let propertyElementID = "property-" + propertyID;
                        propertyElementID = propertyElementID.replace('.', '-');

                        elWrapper.appendChild(createElementFromHTML(`<div class="triple-label">${innerPropertyData.label}</div>`));
                        elProperty.appendChild(elWrapper);

                        let property = createProperty(innerPropertyData, propertyElementID, propertyName, propertyID, elWrapper.childNodes[0]);
                        property.isParticleProperty = group.id.includes("particles");
                        property.elContainer = elContainer;
                        property.spaceMode = propertySpaceMode;
                        
                        if (property.type !== 'placeholder') {
                            properties[propertyID] = property;
                        }
                    }
                } else {
                    let property = createProperty(propertyData, propertyElementID, propertyName, propertyID, elProperty);
                    property.isParticleProperty = group.id.includes("particles");
                    property.elContainer = elContainer;
                    property.spaceMode = propertySpaceMode;
                    
                    if (property.type !== 'placeholder') {
                        properties[propertyID] = property;
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
                }
            });
            
            elGroups[group.id] = elGroup;
        });

        let minorSections = document.querySelectorAll(".section.minor");
        minorSections[minorSections.length - 1].className += " last";

        updateVisibleSpaceModeProperties();
        
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
                        elServerScriptStatus.innerText = ENTITY_SCRIPT_STATUS[data.status] || data.status;
                    } else {
                        elServerScriptStatus.innerText = NOT_RUNNING_SCRIPT_STATUS;
                    }
                } else if (data.type === "update" && data.selections) {
                    if (data.spaceMode !== undefined) {
                        currentSpaceMode = data.spaceMode === "local" ? PROPERTY_SPACE_MODE.LOCAL : PROPERTY_SPACE_MODE.WORLD;
                    }
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

                        let elIcon = properties.type.elSpan;
                        elIcon.innerText = NO_SELECTION;
                        elIcon.style.display = 'inline-block';
                
                        deleteJSONEditor();
                        getPropertyInputElement("userData").value = "";
                        showUserDataTextArea();
                        showSaveUserDataButton();
                        showNewJSONEditorButton();

                        deleteJSONMaterialEditor();
                        getPropertyInputElement("materialData").value = "";
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

                        for (let i = 0; i < selections.length; ++i) {
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
                        typeProperty.elSpan.innerHTML = typeProperty.data.icons[type];
                        typeProperty.elSpan.style.display = "inline-block";
                        
                        disableProperties();
                    } else {
                        selectedEntityProperties = data.selections[0].properties;
                        
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

                        showGroupsForType(selectedEntityProperties.type);
                        
                        for (let propertyID in properties) {
                            let property = properties[propertyID];
                            let propertyData = property.data;
                            let propertyName = property.name;
                            let propertyValue = getPropertyValue(propertyName);
                            
                            let isSubProperty = propertyData.subPropertyOf !== undefined;
                            if (propertyValue === undefined && !isSubProperty) {
                                continue;
                            }
                            
                            let isPropertyNotNumber = false;
                            switch (propertyData.type) {
                                case 'number':
                                    isPropertyNotNumber = isNaN(propertyValue) || propertyValue === null;
                                    break;
                                case 'vec3':
                                case 'vec2':
                                    isPropertyNotNumber = isNaN(propertyValue.x) || propertyValue.x === null;
                                    break;
                                case 'color':
                                    isPropertyNotNumber = isNaN(propertyValue.red) || propertyValue.red === null;
                                    break;
                            }
                            if (isPropertyNotNumber && propertyData.fallbackProperty !== undefined) {
                                propertyValue = getPropertyValue(propertyData.fallbackProperty);
                            }
                            
                            switch (propertyData.type) {
                                case 'string': {
                                    property.elInput.value = propertyValue;
                                    break;
                                }
                                case 'bool': {
                                    let inverse = propertyData.inverse !== undefined ? propertyData.inverse : false;
                                    if (isSubProperty) {
                                        let propertyValue = selectedEntityProperties[propertyData.subPropertyOf];
                                        let subProperties = propertyValue.split(",");
                                        let subPropertyValue = subProperties.indexOf(propertyName) > -1;
                                        property.elInput.checked = inverse ? !subPropertyValue : subPropertyValue;
                                    } else {
                                        property.elInput.checked = inverse ? !propertyValue : propertyValue;
                                    }
                                    break;
                                }
                                case 'number': {
                                    let multiplier = propertyData.multiplier !== undefined ? propertyData.multiplier : 1;
                                    let value = propertyValue / multiplier;
                                    if (propertyData.round !== undefined) {
                                        value = Math.round(value.round) / propertyData.round;
                                    }
                                    property.elNumber.setValue(value);
                                    break;
                                }
                                case 'vec3':
                                case 'vec2': {
                                    let multiplier = propertyData.multiplier !== undefined ? propertyData.multiplier : 1;
                                    let valueX = propertyValue.x / multiplier;
                                    let valueY = propertyValue.y / multiplier;
                                    let valueZ = propertyValue.z / multiplier;
                                    if (propertyData.round !== undefined) {
                                        valueX = Math.round(valueX * propertyData.round) / propertyData.round;
                                        valueY = Math.round(valueY * propertyData.round) / propertyData.round;
                                        valueZ = Math.round(valueZ * propertyData.round) / propertyData.round;
                                    }
                                    if (propertyData.decimals !== undefined) {
                                        property.elNumberX.setValue(valueX.toFixed(propertyData.decimals));
                                        property.elNumberY.setValue(valueY.toFixed(propertyData.decimals));
                                        if (property.elNumberZ !== undefined) {
                                            property.elNumberZ.setValue(valueZ.toFixed(propertyData.decimals));
                                        }
                                    } else {
                                        property.elNumberX.setValue(valueX);
                                        property.elNumberY.setValue(valueY);
                                        if (property.elNumberZ !== undefined) {
                                            property.elNumberZ.setValue(valueZ);
                                        }
                                    }
                                    break;
                                }
                                case 'color': {
                                    property.elColorPicker.style.backgroundColor = "rgb(" + propertyValue.red + "," + 
                                                                                     propertyValue.green + "," + 
                                                                                     propertyValue.blue + ")";
                                    property.elNumberR.setValue(propertyValue.red);
                                    property.elNumberG.setValue(propertyValue.green);
                                    property.elNumberB.setValue(propertyValue.blue);
                                    break;
                                }
                                case 'dropdown': {
                                    property.elInput.value = propertyValue;
                                    setDropdownText(property.elInput);
                                    break;
                                }
                                case 'textarea': {
                                    property.elInput.value = propertyValue;
                                    setTextareaScrolling(property.elInput);
                                    break;
                                }
                                case 'icon': {
                                    property.elSpan.innerHTML = propertyData.icons[propertyValue];
                                    property.elSpan.style.display = "inline-block";
                                    break;
                                }
                                case 'texture': {
                                    property.elInput.value = propertyValue;
                                    property.elInput.imageLoad(property.elInput.value);
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

                        updateVisibleSpaceModeProperties();

                        if (selectedEntityProperties.type === "Image") {
                            let imageLink = JSON.parse(selectedEntityProperties.textures)["tex.picture"];
                            getPropertyInputElement("image").value = imageLink;
                        } else if (selectedEntityProperties.type === "Material") {
                            let elParentMaterialNameString = getPropertyInputElement("materialNameToReplace");
                            let elParentMaterialNameNumber = getPropertyInputElement("submeshToReplace");
                            let elParentMaterialNameCheckbox = getPropertyInputElement("selectSubmesh");
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
                            getPropertyInputElement("userData").value = selectedEntityProperties.userData;
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
                            getPropertyInputElement("materialData").value = selectedEntityProperties.materialData;
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
                            getPropertyInputElement("locked").removeAttribute('disabled');
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
                } else if (data.type === 'tooltipsReply') {
                    createAppTooltip.setIsEnabled(!data.hmdActive);
                    createAppTooltip.setTooltipData(data.tooltips);
                } else if (data.type === 'hmdActiveChanged') {
                    createAppTooltip.setIsEnabled(!data.hmdActive);
                } else if (data.type === 'setSpaceMode') {
                    currentSpaceMode = data.spaceMode === "local" ? PROPERTY_SPACE_MODE.LOCAL : PROPERTY_SPACE_MODE.WORLD;
                    updateVisibleSpaceModeProperties();
                }
            });

            // Request tooltips as soon as we can process a reply:
            EventBridge.emitWebEvent(JSON.stringify({ type: 'tooltipsRequest' }));
        }
        
        // Server Script Status
        let elServerScriptStatusOuter = document.getElementById('div-property-serverScriptStatus');
        let elServerScriptStatusContainer = document.getElementById('div-property-serverScriptStatus').childNodes[1];
        let serverScriptStatusElementID = 'property-serverScripts-status';
        createAppTooltip.registerTooltipElement(elServerScriptStatusOuter.childNodes[0], "serverScriptsStatus");
        let elServerScriptStatus = document.createElement('div');
        elServerScriptStatus.setAttribute("id", serverScriptStatusElementID);
        elServerScriptStatusContainer.appendChild(elServerScriptStatus);
        
        // Server Script Error
        let elServerScripts = getPropertyInputElement("serverScripts");
        elDiv = document.createElement('div');
        elDiv.className = "property";
        let elServerScriptError = document.createElement('textarea');
        let serverScriptErrorElementID = 'property-serverScripts-error';
        elServerScriptError.setAttribute("id", serverScriptErrorElementID);
        elDiv.appendChild(elServerScriptError);
        elServerScriptStatusContainer.appendChild(elDiv);
        
        let elScript = getPropertyInputElement("script");
        elScript.parentNode.className = "url refresh";
        elServerScripts.parentNode.className = "url refresh";
            
        // User Data
        let userDataProperty = properties["userData"];
        let elUserData = userDataProperty.elInput;
        let userDataElementID = userDataProperty.elementID;
        elDiv = elUserData.parentNode;
        let elStaticUserData = document.createElement('div');
        elStaticUserData.setAttribute("id", userDataElementID + "-static");
        let elUserDataEditor = document.createElement('div');
        elUserDataEditor.setAttribute("id", userDataElementID + "-editor");
        let elUserDataSaved = document.createElement('span');
        elUserDataSaved.setAttribute("id", userDataElementID + "-saved");
        elUserDataSaved.innerText = "Saved!";
        elDiv.childNodes[JSON_EDITOR_ROW_DIV_INDEX].appendChild(elUserDataSaved);
        elDiv.insertBefore(elStaticUserData, elUserData);
        elDiv.insertBefore(elUserDataEditor, elUserData);
        
        // Material Data
        let materialDataProperty = properties["materialData"];
        let elMaterialData = materialDataProperty.elInput;
        let materialDataElementID = materialDataProperty.elementID;
        elDiv = elMaterialData.parentNode;
        let elStaticMaterialData = document.createElement('div');
        elStaticMaterialData.setAttribute("id", materialDataElementID + "-static");
        let elMaterialDataEditor = document.createElement('div');
        elMaterialDataEditor.setAttribute("id", materialDataElementID + "-editor");
        let elMaterialDataSaved = document.createElement('span');
        elMaterialDataSaved.setAttribute("id", materialDataElementID + "-saved");
        elMaterialDataSaved.innerText = "Saved!";
        elDiv.childNodes[JSON_EDITOR_ROW_DIV_INDEX].appendChild(elMaterialDataSaved);
        elDiv.insertBefore(elStaticMaterialData, elMaterialData);
        elDiv.insertBefore(elMaterialDataEditor, elMaterialData);
        
        // Special Property Callbacks
        let elParentMaterialNameString = getPropertyInputElement("materialNameToReplace");
        let elParentMaterialNameNumber = getPropertyInputElement("submeshToReplace");
        let elParentMaterialNameCheckbox = getPropertyInputElement("selectSubmesh");
        elParentMaterialNameString.addEventListener('change', function () { 
            updateProperty("parentMaterialName", MATERIAL_PREFIX_STRING + this.value, false); 
        });
        elParentMaterialNameNumber.addEventListener('change', function () { 
            updateProperty("parentMaterialName", this.value, false); 
        });
        elParentMaterialNameCheckbox.addEventListener('change', function () {
            if (this.checked) {
                updateProperty("parentMaterialName", elParentMaterialNameNumber.value, false);
                showParentMaterialNameBox(true, elParentMaterialNameNumber, elParentMaterialNameString);
            } else {
                updateProperty("parentMaterialName", MATERIAL_PREFIX_STRING + elParentMaterialNameString.value, false);
                showParentMaterialNameBox(false, elParentMaterialNameNumber, elParentMaterialNameString);
            }
        });
        
        getPropertyInputElement("image").addEventListener('change', createImageURLUpdateFunction(properties['textures']));
        
        // Collapsible sections
        let elCollapsible = document.getElementsByClassName("collapse-icon");

        let toggleCollapsedEvent = function(event) {
            let element = this.parentNode.parentNode;
            let isCollapsed = element.dataset.collapsed !== "true";
            element.dataset.collapsed = isCollapsed ? "true" : false;
            element.setAttribute("collapsed", isCollapsed ? "true" : "false");
            this.textContent = isCollapsed ? "L" : "M";
        };

        for (let collapseIndex = 0, numCollapsibles = elCollapsible.length; collapseIndex < numCollapsibles; ++collapseIndex) {
            let curCollapsibleElement = elCollapsible[collapseIndex];
            curCollapsibleElement.addEventListener("click", toggleCollapsedEvent, true);
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
            let elDropdown = elDropdowns[dropDownIndex];
            let options = elDropdown.getElementsByTagName("option");
            let selectedOption = 0;
            for (let optionIndex = 0; optionIndex < options.length; ++optionIndex) {
                if (options[optionIndex].getAttribute("selected") === "selected") {
                    selectedOption = optionIndex;
                    break;
                }
            }
            let div = elDropdown.parentNode;

            let dl = document.createElement("dl");
            div.appendChild(dl);

            let dt = document.createElement("dt");
            dt.name = elDropdown.name;
            dt.id = elDropdown.id;
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
            
            let propertyID = elDropdown.getAttribute("propertyID");
            let property = properties[propertyID];
            property.elInput = dt;
            dt.addEventListener('change', createEmitTextPropertyUpdateFunction(property));
        }
        
        elDropdowns = document.getElementsByTagName("select");
        while (elDropdowns.length > 0) {
            let el = elDropdowns[0];
            el.parentNode.removeChild(el);
            elDropdowns = document.getElementsByTagName("select");
        }

        const KEY_CODES = {
            BACKSPACE: 8,
            DELETE: 46
        };

        document.addEventListener("keyup", function (keyUpEvent) {
            if (keyUpEvent.target.nodeName === "INPUT") {
                return;
            }
            let {code, key, keyCode, altKey, ctrlKey, metaKey, shiftKey} = keyUpEvent;

            let controlKey = window.navigator.platform.startsWith("Mac") ? metaKey : ctrlKey;

            let keyCodeString;
            switch (keyCode) {
                case KEY_CODES.DELETE:
                    keyCodeString = "Delete";
                    break;
                case KEY_CODES.BACKSPACE:
                    keyCodeString = "Backspace";
                    break;
                default:
                    keyCodeString = String.fromCharCode(keyUpEvent.keyCode);
                    break;
            }

            EventBridge.emitWebEvent(JSON.stringify({
                type: 'keyUpEvent',
                keyUpEvent: {
                    code,
                    key,
                    keyCode,
                    keyCodeString,
                    altKey,
                    controlKey,
                    shiftKey,
                }
            }));
        }, false);
        
        window.onblur = function() {
            // Fake a change event
            let ev = document.createEvent("HTMLEvents");
            ev.initEvent("change", true, true);
            document.activeElement.dispatchEvent(ev);
        };
        
        // For input and textarea elements, select all of the text on focus
        let els = document.querySelectorAll("input, textarea");
        for (let i = 0; i < els.length; ++i) {
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
