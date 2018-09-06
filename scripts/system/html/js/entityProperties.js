//  entityProperties.js
//
//  Created by Ryan Huffman on 13 Nov 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global alert, augmentSpinButtons, clearTimeout, console, document, Element, EventBridge, 
    HifiEntityUI, JSONEditor, openEventBridge, setTimeout, window, _ $ */

var PI = 3.14159265358979;
var DEGREES_TO_RADIANS = PI / 180.0;
var RADIANS_TO_DEGREES = 180.0 / PI;
var ICON_FOR_TYPE = {
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

var EDITOR_TIMEOUT_DURATION = 1500;
var KEY_P = 80; // Key code for letter p used for Parenting hotkey.
var colorPickers = {};
var lastEntityID = null;

var MATERIAL_PREFIX_STRING = "mat::";

var PENDING_SCRIPT_STATUS = "[ Fetching status ]";

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
    var elLocked = document.getElementById("property-locked");

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
    var elLocked = document.getElementById("property-locked");

    if (elLocked.checked === true) {
        if ($('#userdata-editor').css('display') === "block") {
            showStaticUserData();
        }
        if ($('#materialdata-editor').css('display') === "block") {
            showStaticMaterialData();
        }
    }
}

function showElements(els, show) {
    for (var i = 0; i < els.length; i++) {
        els[i].style.display = (show) ? 'table' : 'none';
    }
}

function updateProperty(propertyName, propertyValue) {
    var properties = {};
    properties[propertyName] = propertyValue;
    updateProperties(properties);
}

function updateProperties(properties) {
    EventBridge.emitWebEvent(JSON.stringify({
        id: lastEntityID,
        type: "update",
        properties: properties
    }));
}

function createEmitCheckedPropertyUpdateFunction(propertyName) {
    return function() {
        updateProperty(propertyName, this.checked);
    };
}

function createEmitGroupCheckedPropertyUpdateFunction(group, propertyName) {
    return function() {
        var properties = {};
        properties[group] = {};
        properties[group][propertyName] = this.checked;
        updateProperties(properties);
    };
}

function createEmitNumberPropertyUpdateFunction(propertyName, decimals) {
    decimals = ((decimals === undefined) ? 4 : decimals);
    return function() {
        var value = parseFloat(this.value).toFixed(decimals);
        updateProperty(propertyName, value);
    };
}

function createEmitGroupNumberPropertyUpdateFunction(group, propertyName) {
    return function() {
        var properties = {};
        properties[group] = {};
        properties[group][propertyName] = this.value;
        updateProperties(properties);
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

function createZoneComponentModeChangedFunction(zoneComponent, zoneComponentModeInherit,
    zoneComponentModeDisabled, zoneComponentModeEnabled) {

    return function() {
        var zoneComponentMode;

        if (zoneComponentModeInherit.checked) {
            zoneComponentMode = 'inherit';
        } else if (zoneComponentModeDisabled.checked) {
            zoneComponentMode = 'disabled';
        } else if (zoneComponentModeEnabled.checked) {
            zoneComponentMode = 'enabled';
        }

        updateProperty(zoneComponent, zoneComponentMode);
    };
}

function createEmitGroupTextPropertyUpdateFunction(group, propertyName) {
    return function() {
        var properties = {};
        properties[group] = {};
        properties[group][propertyName] = this.value;
        updateProperties(properties);
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

function createEmitVec3PropertyUpdateFunction(property, elX, elY, elZ) {
    return function() {
        var properties = {};
        properties[property] = {
            x: elX.value,
            y: elY.value,
            z: elZ.value
        };
        updateProperties(properties);
    };
}

function createEmitGroupVec3PropertyUpdateFunction(group, property, elX, elY, elZ) {
    return function() {
        var properties = {};
        properties[group] = {};
        properties[group][property] = {
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


function createEmitGroupColorPropertyUpdateFunction(group, property, elRed, elGreen, elBlue) {
    return function() {
        var properties = {};
        properties[group] = {};
        properties[group][property] = {
            red: elRed.value,
            green: elGreen.value,
            blue: elBlue.value
        };
        updateProperties(properties);
    };
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
        if ($('#userdata-editor').css('height') !== "0px") {
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

function setTextareaScrolling(element) {
    var isScrolling = element.scrollHeight > element.offsetHeight;
    element.setAttribute("scrolling", isScrolling ? "true" : "false");
}


var materialEditor = null;

function createJSONMaterialEditor() {
    var container = document.getElementById("materialdata-editor");
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
            $('#materialdata-save').attr('disabled', false);


        }
    };
    materialEditor = new JSONEditor(container, options);
}

function hideNewJSONMaterialEditorButton() {
    $('#materialdata-new-editor').hide();
}

function showSaveMaterialDataButton() {
    $('#materialdata-save').show();
}

function hideSaveMaterialDataButton() {
    $('#materialdata-save').hide();
}

function showNewJSONMaterialEditorButton() {
    $('#materialdata-new-editor').show();
}

function showMaterialDataTextArea() {
    $('#property-material-data').show();
}

function hideMaterialDataTextArea() {
    $('#property-material-data').hide();
}

function showStaticMaterialData() {
    if (materialEditor !== null) {
        $('#static-materialdata').show();
        $('#static-materialdata').css('height', $('#materialdata-editor').height());
        $('#static-materialdata').text(materialEditor.getText());
    }
}

function removeStaticMaterialData() {
    $('#static-materialdata').hide();
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
    $('#materialdata-saved').show();
    $('#materialdata-save').attr('disabled', true);
    if (savedMaterialJSONTimer !== null) {
        clearTimeout(savedMaterialJSONTimer);
    }
    savedMaterialJSONTimer = setTimeout(function() {
        $('#materialdata-saved').hide();

    }, EDITOR_TIMEOUT_DURATION);
}

var editor = null;

function createJSONEditor() {
    var container = document.getElementById("userdata-editor");
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
            $('#userdata-save').attr('disabled', false);


        }
    };
    editor = new JSONEditor(container, options);
}

function hideNewJSONEditorButton() {
    $('#userdata-new-editor').hide();
}

function showSaveUserDataButton() {
    $('#userdata-save').show();
}

function hideSaveUserDataButton() {
    $('#userdata-save').hide();
}

function showNewJSONEditorButton() {
    $('#userdata-new-editor').show();
}

function showUserDataTextArea() {
    $('#property-user-data').show();
}

function hideUserDataTextArea() {
    $('#property-user-data').hide();
}

function showStaticUserData() {
    if (editor !== null) {
        $('#static-userdata').show();
        $('#static-userdata').css('height', $('#userdata-editor').height());
        $('#static-userdata').text(editor.getText());
    }
}

function removeStaticUserData() {
    $('#static-userdata').hide();
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
    $('#userdata-saved').show();
    $('#userdata-save').attr('disabled', true);
    if (savedJSONTimer !== null) {
        clearTimeout(savedJSONTimer);
    }
    savedJSONTimer = setTimeout(function() {
        $('#userdata-saved').hide();

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
            if (e.target.id === "userdata-new-editor" || e.target.id === "userdata-clear" || e.target.id === "materialdata-new-editor" || e.target.id === "materialdata-clear") {
                return;
            } else {
                if ($('#userdata-editor').css('height') !== "0px") {
                    saveJSONUserData(true);
                }
                if ($('#materialdata-editor').css('height') !== "0px") {
                    saveJSONMaterialData(true);
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

function showParentMaterialNameBox(number, elNumber, elString) {
    if (number) {
        $('#property-parent-material-id-number-container').show();
        $('#property-parent-material-id-string-container').hide();
        elString.value = "";
    } else {
        $('#property-parent-material-id-string-container').show();
        $('#property-parent-material-id-number-container').hide();
        elNumber.value = 0;
    }
}

function loaded() {
    openEventBridge(function() {

        var elPropertiesList = document.getElementById("properties-list");
        var elID = document.getElementById("property-id");
        var elType = document.getElementById("property-type");
        var elTypeIcon = document.getElementById("type-icon");
        var elName = document.getElementById("property-name");
        var elLocked = document.getElementById("property-locked");
        var elVisible = document.getElementById("property-visible");
        var elPositionX = document.getElementById("property-pos-x");
        var elPositionY = document.getElementById("property-pos-y");
        var elPositionZ = document.getElementById("property-pos-z");
        var elMoveSelectionToGrid = document.getElementById("move-selection-to-grid");
        var elMoveAllToGrid = document.getElementById("move-all-to-grid");

        var elDimensionsX = document.getElementById("property-dim-x");
        var elDimensionsY = document.getElementById("property-dim-y");
        var elDimensionsZ = document.getElementById("property-dim-z");
        var elResetToNaturalDimensions = document.getElementById("reset-to-natural-dimensions");
        var elRescaleDimensionsPct = document.getElementById("dimension-rescale-pct");
        var elRescaleDimensionsButton = document.getElementById("dimension-rescale-button");

        var elParentID = document.getElementById("property-parent-id");
        var elParentJointIndex = document.getElementById("property-parent-joint-index");

        var elRegistrationX = document.getElementById("property-reg-x");
        var elRegistrationY = document.getElementById("property-reg-y");
        var elRegistrationZ = document.getElementById("property-reg-z");

        var elRotationX = document.getElementById("property-rot-x");
        var elRotationY = document.getElementById("property-rot-y");
        var elRotationZ = document.getElementById("property-rot-z");

        var elLinearVelocityX = document.getElementById("property-lvel-x");
        var elLinearVelocityY = document.getElementById("property-lvel-y");
        var elLinearVelocityZ = document.getElementById("property-lvel-z");
        var elLinearDamping = document.getElementById("property-ldamping");

        var elAngularVelocityX = document.getElementById("property-avel-x");
        var elAngularVelocityY = document.getElementById("property-avel-y");
        var elAngularVelocityZ = document.getElementById("property-avel-z");
        var elAngularDamping = document.getElementById("property-adamping");

        var elRestitution = document.getElementById("property-restitution");
        var elFriction = document.getElementById("property-friction");

        var elGravityX = document.getElementById("property-grav-x");
        var elGravityY = document.getElementById("property-grav-y");
        var elGravityZ = document.getElementById("property-grav-z");

        var elAccelerationX = document.getElementById("property-lacc-x");
        var elAccelerationY = document.getElementById("property-lacc-y");
        var elAccelerationZ = document.getElementById("property-lacc-z");

        var elDensity = document.getElementById("property-density");
        var elCollisionless = document.getElementById("property-collisionless");
        var elDynamic = document.getElementById("property-dynamic");
        var elCollideStatic = document.getElementById("property-collide-static");
        var elCollideDynamic = document.getElementById("property-collide-dynamic");
        var elCollideKinematic = document.getElementById("property-collide-kinematic");
        var elCollideMyAvatar = document.getElementById("property-collide-myAvatar");
        var elCollideOtherAvatar = document.getElementById("property-collide-otherAvatar");
        var elCollisionSoundURL = document.getElementById("property-collision-sound-url");

        var elGrabbable = document.getElementById("property-grabbable");

        var elCloneable = document.getElementById("property-cloneable");
        var elCloneableDynamic = document.getElementById("property-cloneable-dynamic");
        var elCloneableAvatarEntity = document.getElementById("property-cloneable-avatarEntity");
        var elCloneableGroup = document.getElementById("group-cloneable-group");
        var elCloneableLifetime = document.getElementById("property-cloneable-lifetime");
        var elCloneableLimit = document.getElementById("property-cloneable-limit");

        var elTriggerable = document.getElementById("property-triggerable");
        var elIgnoreIK = document.getElementById("property-ignore-ik");

        var elLifetime = document.getElementById("property-lifetime");
        var elScriptURL = document.getElementById("property-script-url");
        var elScriptTimestamp = document.getElementById("property-script-timestamp");
        var elReloadScriptsButton = document.getElementById("reload-script-button");
        var elServerScripts = document.getElementById("property-server-scripts");
        var elReloadServerScriptsButton = document.getElementById("reload-server-scripts-button");
        var elServerScriptStatus = document.getElementById("server-script-status");
        var elServerScriptError = document.getElementById("server-script-error");
        var elUserData = document.getElementById("property-user-data");
        var elClearUserData = document.getElementById("userdata-clear");
        var elSaveUserData = document.getElementById("userdata-save");
        var elNewJSONEditor = document.getElementById('userdata-new-editor');
        var elColorControlVariant2 = document.getElementById("property-color-control2");
        var elColorRed = document.getElementById("property-color-red");
        var elColorGreen = document.getElementById("property-color-green");
        var elColorBlue = document.getElementById("property-color-blue");

        var elShape = document.getElementById("property-shape");

        var elCanCastShadow = document.getElementById("property-can-cast-shadow");

        var elLightSpotLight = document.getElementById("property-light-spot-light");
        var elLightColor = document.getElementById("property-light-color");
        var elLightColorRed = document.getElementById("property-light-color-red");
        var elLightColorGreen = document.getElementById("property-light-color-green");
        var elLightColorBlue = document.getElementById("property-light-color-blue");

        var elLightIntensity = document.getElementById("property-light-intensity");
        var elLightFalloffRadius = document.getElementById("property-light-falloff-radius");
        var elLightExponent = document.getElementById("property-light-exponent");
        var elLightCutoff = document.getElementById("property-light-cutoff");

        var elModelURL = document.getElementById("property-model-url");
        var elShapeType = document.getElementById("property-shape-type");
        var elCompoundShapeURL = document.getElementById("property-compound-shape-url");
        var elModelAnimationURL = document.getElementById("property-model-animation-url");
        var elModelAnimationPlaying = document.getElementById("property-model-animation-playing");
        var elModelAnimationFPS = document.getElementById("property-model-animation-fps");
        var elModelAnimationFrame = document.getElementById("property-model-animation-frame");
        var elModelAnimationFirstFrame = document.getElementById("property-model-animation-first-frame");
        var elModelAnimationLastFrame = document.getElementById("property-model-animation-last-frame");
        var elModelAnimationLoop = document.getElementById("property-model-animation-loop");
        var elModelAnimationHold = document.getElementById("property-model-animation-hold");
        var elModelAnimationAllowTranslation = document.getElementById("property-model-animation-allow-translation");
        var elModelTextures = document.getElementById("property-model-textures");
        var elModelOriginalTextures = document.getElementById("property-model-original-textures");

        var elMaterialURL = document.getElementById("property-material-url");
        //var elMaterialMappingMode = document.getElementById("property-material-mapping-mode");
        var elPriority = document.getElementById("property-priority");
        var elParentMaterialNameString = document.getElementById("property-parent-material-id-string");
        var elParentMaterialNameNumber = document.getElementById("property-parent-material-id-number");
        var elParentMaterialNameCheckbox = document.getElementById("property-parent-material-id-checkbox");
        var elMaterialMappingPosX = document.getElementById("property-material-mapping-pos-x");
        var elMaterialMappingPosY = document.getElementById("property-material-mapping-pos-y");
        var elMaterialMappingScaleX = document.getElementById("property-material-mapping-scale-x");
        var elMaterialMappingScaleY = document.getElementById("property-material-mapping-scale-y");
        var elMaterialMappingRot = document.getElementById("property-material-mapping-rot");
        var elMaterialData = document.getElementById("property-material-data");
        var elClearMaterialData = document.getElementById("materialdata-clear");
        var elSaveMaterialData = document.getElementById("materialdata-save");
        var elNewJSONMaterialEditor = document.getElementById('materialdata-new-editor');

        var elImageURL = document.getElementById("property-image-url");

        var elWebSourceURL = document.getElementById("property-web-source-url");
        var elWebDPI = document.getElementById("property-web-dpi");

        var elDescription = document.getElementById("property-description");

        var elHyperlinkHref = document.getElementById("property-hyperlink-href");

        var elTextText = document.getElementById("property-text-text");
        var elTextLineHeight = document.getElementById("property-text-line-height");
        var elTextTextColor = document.getElementById("property-text-text-color");
        var elTextFaceCamera = document.getElementById("property-text-face-camera");
        var elTextTextColorRed = document.getElementById("property-text-text-color-red");
        var elTextTextColorGreen = document.getElementById("property-text-text-color-green");
        var elTextTextColorBlue = document.getElementById("property-text-text-color-blue");
        var elTextBackgroundColor = document.getElementById("property-text-background-color");
        var elTextBackgroundColorRed = document.getElementById("property-text-background-color-red");
        var elTextBackgroundColorGreen = document.getElementById("property-text-background-color-green");
        var elTextBackgroundColorBlue = document.getElementById("property-text-background-color-blue");

        // Key light
        var elZoneKeyLightModeInherit = document.getElementById("property-zone-key-light-mode-inherit");
        var elZoneKeyLightModeDisabled = document.getElementById("property-zone-key-light-mode-disabled");
        var elZoneKeyLightModeEnabled = document.getElementById("property-zone-key-light-mode-enabled");

        var elZoneKeyLightColor = document.getElementById("property-zone-key-light-color");
        var elZoneKeyLightColorRed = document.getElementById("property-zone-key-light-color-red");
        var elZoneKeyLightColorGreen = document.getElementById("property-zone-key-light-color-green");
        var elZoneKeyLightColorBlue = document.getElementById("property-zone-key-light-color-blue");
        var elZoneKeyLightIntensity = document.getElementById("property-zone-key-intensity");
        var elZoneKeyLightDirectionX = document.getElementById("property-zone-key-light-direction-x");
        var elZoneKeyLightDirectionY = document.getElementById("property-zone-key-light-direction-y");

        var elZoneKeyLightCastShadows = document.getElementById("property-zone-key-light-cast-shadows");

        // Skybox
        var elZoneSkyboxModeInherit = document.getElementById("property-zone-skybox-mode-inherit");
        var elZoneSkyboxModeDisabled = document.getElementById("property-zone-skybox-mode-disabled");
        var elZoneSkyboxModeEnabled = document.getElementById("property-zone-skybox-mode-enabled");

        // Ambient light
        var elCopySkyboxURLToAmbientURL = document.getElementById("copy-skybox-url-to-ambient-url");

        var elZoneAmbientLightModeInherit = document.getElementById("property-zone-ambient-light-mode-inherit");
        var elZoneAmbientLightModeDisabled = document.getElementById("property-zone-ambient-light-mode-disabled");
        var elZoneAmbientLightModeEnabled = document.getElementById("property-zone-ambient-light-mode-enabled");

        var elZoneAmbientLightIntensity = document.getElementById("property-zone-key-ambient-intensity");
        var elZoneAmbientLightURL = document.getElementById("property-zone-key-ambient-url");

        // Haze
        var elZoneHazeModeInherit = document.getElementById("property-zone-haze-mode-inherit");
        var elZoneHazeModeDisabled = document.getElementById("property-zone-haze-mode-disabled");
        var elZoneHazeModeEnabled = document.getElementById("property-zone-haze-mode-enabled");

        var elZoneHazeRange = document.getElementById("property-zone-haze-range");
        var elZoneHazeColor = document.getElementById("property-zone-haze-color");
        var elZoneHazeColorRed = document.getElementById("property-zone-haze-color-red");
        var elZoneHazeColorGreen = document.getElementById("property-zone-haze-color-green");
        var elZoneHazeColorBlue = document.getElementById("property-zone-haze-color-blue");
        var elZoneHazeGlareColor = document.getElementById("property-zone-haze-glare-color");
        var elZoneHazeGlareColorRed = document.getElementById("property-zone-haze-glare-color-red");
        var elZoneHazeGlareColorGreen = document.getElementById("property-zone-haze-glare-color-green");
        var elZoneHazeGlareColorBlue = document.getElementById("property-zone-haze-glare-color-blue");
        var elZoneHazeEnableGlare = document.getElementById("property-zone-haze-enable-light-blend");
        var elZoneHazeGlareAngle = document.getElementById("property-zone-haze-blend-angle");
        
        var elZoneHazeAltitudeEffect = document.getElementById("property-zone-haze-altitude-effect");
        var elZoneHazeBaseRef = document.getElementById("property-zone-haze-base");
        var elZoneHazeCeiling = document.getElementById("property-zone-haze-ceiling");

        var elZoneHazeBackgroundBlend = document.getElementById("property-zone-haze-background-blend");

        // Bloom
        var elZoneBloomModeInherit = document.getElementById("property-zone-bloom-mode-inherit");
        var elZoneBloomModeDisabled = document.getElementById("property-zone-bloom-mode-disabled");
        var elZoneBloomModeEnabled = document.getElementById("property-zone-bloom-mode-enabled");

        var elZoneBloomIntensity = document.getElementById("property-zone-bloom-intensity");
        var elZoneBloomThreshold = document.getElementById("property-zone-bloom-threshold");
        var elZoneBloomSize = document.getElementById("property-zone-bloom-size");

        var elZoneSkyboxColor = document.getElementById("property-zone-skybox-color");
        var elZoneSkyboxColorRed = document.getElementById("property-zone-skybox-color-red");
        var elZoneSkyboxColorGreen = document.getElementById("property-zone-skybox-color-green");
        var elZoneSkyboxColorBlue = document.getElementById("property-zone-skybox-color-blue");
        var elZoneSkyboxURL = document.getElementById("property-zone-skybox-url");

        var elZoneFlyingAllowed = document.getElementById("property-zone-flying-allowed");
        var elZoneGhostingAllowed = document.getElementById("property-zone-ghosting-allowed");
        var elZoneFilterURL = document.getElementById("property-zone-filter-url");

        var elVoxelVolumeSizeX = document.getElementById("property-voxel-volume-size-x");
        var elVoxelVolumeSizeY = document.getElementById("property-voxel-volume-size-y");
        var elVoxelVolumeSizeZ = document.getElementById("property-voxel-volume-size-z");
        var elVoxelSurfaceStyle = document.getElementById("property-voxel-surface-style");
        var elXTextureURL = document.getElementById("property-x-texture-url");
        var elYTextureURL = document.getElementById("property-y-texture-url");
        var elZTextureURL = document.getElementById("property-z-texture-url");

        if (window.EventBridge !== undefined) {
            var properties;
            EventBridge.scriptEventReceived.connect(function(data) {
                data = JSON.parse(data);
                if (data.type === "server_script_status") {
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
                                saveJSONUserData(true);
                                deleteJSONEditor();
                            }
                            if (materialEditor !== null) {
                                saveJSONMaterialData(true);
                                deleteJSONMaterialEditor();
                            }
                        }

                        elTypeIcon.style.display = "none";
                        elType.innerHTML = "<i>No selection</i>";
                        elPropertiesList.className = '';

                        elID.value = "";
                        elName.value = "";
                        elLocked.checked = false;
                        elVisible.checked = false;

                        elParentID.value = "";
                        elParentJointIndex.value = "";

                        elColorRed.value = "";
                        elColorGreen.value = "";
                        elColorBlue.value = "";
                        elColorControlVariant2.style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";

                        elPositionX.value = "";
                        elPositionY.value = "";
                        elPositionZ.value = "";

                        elRotationX.value = "";
                        elRotationY.value = "";
                        elRotationZ.value = "";

                        elDimensionsX.value = "";
                        elDimensionsY.value = "";
                        elDimensionsZ.value = "";   

                        elRegistrationX.value = "";
                        elRegistrationY.value = "";
                        elRegistrationZ.value = "";

                        elLinearVelocityX.value = "";
                        elLinearVelocityY.value = "";
                        elLinearVelocityZ.value = "";
                        elLinearDamping.value = "";

                        elAngularVelocityX.value = "";
                        elAngularVelocityY.value = "";
                        elAngularVelocityZ.value = "";
                        elAngularDamping.value = "";

                        elGravityX.value = "";
                        elGravityY.value = "";
                        elGravityZ.value = "";

                        elAccelerationX.value = "";
                        elAccelerationY.value = "";
                        elAccelerationZ.value = "";

                        elRestitution.value = "";
                        elFriction.value = "";
                        elDensity.value = "";

                        elCollisionless.checked = false;
                        elDynamic.checked = false;

                        elCollideStatic.checked = false;
                        elCollideKinematic.checked = false;
                        elCollideDynamic.checked = false;
                        elCollideMyAvatar.checked = false;
                        elCollideOtherAvatar.checked = false;

                        elGrabbable.checked = false;
                        elTriggerable.checked = false;
                        elIgnoreIK.checked = false;

                        elCloneable.checked = false;
                        elCloneableDynamic.checked = false;
                        elCloneableAvatarEntity.checked = false;
                        elCloneableGroup.style.display = "none";
                        elCloneableLimit.value = "";
                        elCloneableLifetime.value = "";

                        showElements(document.getElementsByClassName('can-cast-shadow-section'), true);
                        elCanCastShadow.checked = false;

                        elCollisionSoundURL.value = "";
                        elLifetime.value = "";
                        elScriptURL.value = "";
                        elServerScripts.value = "";
                        elHyperlinkHref.value = "";
                        elDescription.value = "";

                        deleteJSONEditor();
                        elUserData.value = "";
                        showUserDataTextArea();
                        showSaveUserDataButton();
                        showNewJSONEditorButton();

                        // Shape Properties
                        elShape.value = "Cube";
                        setDropdownText(elShape);

                        // Light Properties
                        elLightSpotLight.checked = false;
                        elLightColor.style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                        elLightColorRed.value = "";
                        elLightColorGreen.value = "";
                        elLightColorBlue.value = "";
                        elLightIntensity.value = "";
                        elLightFalloffRadius.value = "";
                        elLightExponent.value = "";
                        elLightCutoff.value = "";

                        // Model Properties
                        elModelURL.value = "";
                        elCompoundShapeURL.value = "";
                        elShapeType.value = "none";
                        setDropdownText(elShapeType);
                        elModelAnimationURL.value = ""
                        elModelAnimationPlaying.checked = false;
                        elModelAnimationFPS.value = "";
                        elModelAnimationFrame.value = "";
                        elModelAnimationFirstFrame.value = "";
                        elModelAnimationLastFrame.value = "";
                        elModelAnimationLoop.checked = false;
                        elModelAnimationHold.checked = false;
                        elModelAnimationAllowTranslation.checked = false;
                        elModelTextures.value = "";
                        elModelOriginalTextures.value = "";

                        // Zone Properties
                        elZoneFlyingAllowed.checked = false;
                        elZoneGhostingAllowed.checked = false;
                        elZoneFilterURL.value = "";
                        elZoneKeyLightColor.style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                        elZoneKeyLightColorRed.value = "";
                        elZoneKeyLightColorGreen.value = "";
                        elZoneKeyLightColorBlue.value = "";
                        elZoneKeyLightIntensity.value = "";
                        elZoneKeyLightDirectionX.value = "";
                        elZoneKeyLightDirectionY.value = "";
                        elZoneKeyLightCastShadows.checked = false;
                        elZoneAmbientLightIntensity.value = "";
                        elZoneAmbientLightURL.value = "";
                        elZoneHazeRange.value = "";
                        elZoneHazeColor.style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                        elZoneHazeColorRed.value = "";
                        elZoneHazeColorGreen.value = "";
                        elZoneHazeColorBlue.value = "";
                        elZoneHazeBackgroundBlend.value = 0;
                        elZoneHazeGlareColor.style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                        elZoneHazeGlareColorRed.value = "";
                        elZoneHazeGlareColorGreen.value = "";
                        elZoneHazeGlareColorBlue.value = "";
                        elZoneHazeEnableGlare.checked = false;
                        elZoneHazeGlareAngle.value = "";
                        elZoneHazeAltitudeEffect.checked = false;
                        elZoneHazeBaseRef.value = "";
                        elZoneHazeCeiling.value = "";
                        elZoneBloomIntensity.value = "";
                        elZoneBloomThreshold.value = "";
                        elZoneBloomSize.value = "";
                        elZoneSkyboxColor.style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                        elZoneSkyboxColorRed.value = "";
                        elZoneSkyboxColorGreen.value = "";
                        elZoneSkyboxColorBlue.value = "";
                        elZoneSkyboxURL.value = "";
                        showElements(document.getElementsByClassName('keylight-section'), true);
                        showElements(document.getElementsByClassName('skybox-section'), true);
                        showElements(document.getElementsByClassName('ambient-section'), true);
                        showElements(document.getElementsByClassName('haze-section'), true);
                        showElements(document.getElementsByClassName('bloom-section'), true);

                        // Text Properties
                        elTextText.value = "";
                        elTextLineHeight.value = "";
                        elTextFaceCamera.checked = false;
                        elTextTextColor.style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                        elTextTextColorRed.value = "";
                        elTextTextColorGreen.value = "";
                        elTextTextColorBlue.value = "";
                        elTextBackgroundColor.style.backgroundColor = "rgb(" + 0 + "," + 0 + "," + 0 + ")";
                        elTextBackgroundColorRed.value = "";
                        elTextBackgroundColorGreen.value = "";
                        elTextBackgroundColorBlue.value = "";

                        // Image Properties
                        elImageURL.value = "";

                        // Web Properties
                        elWebSourceURL.value = "";
                        elWebDPI.value = "";

                        // Material Properties
                        elMaterialURL.value = "";
                        elParentMaterialNameNumber.value = "";
                        elParentMaterialNameCheckbox.checked = false;
                        elPriority.value = "";
                        elMaterialMappingPosX.value = "";
                        elMaterialMappingPosY.value = "";
                        elMaterialMappingScaleX.value = "";
                        elMaterialMappingScaleY.value = "";
                        elMaterialMappingRot.value = "";

                        deleteJSONMaterialEditor();
                        elMaterialData.value = "";
                        showMaterialDataTextArea();
                        showSaveMaterialDataButton();
                        showNewJSONMaterialEditorButton();

                        disableProperties();
                    } else if (data.selections.length > 1) {
                        deleteJSONEditor();
                        deleteJSONMaterialEditor();
                        var selections = data.selections;

                        var ids = [];
                        var types = {};
                        var numTypes = 0;

                        for (var i = 0; i < selections.length; i++) {
                            ids.push(selections[i].id);
                            var currentSelectedType = selections[i].properties.type;
                            if (types[currentSelectedType] === undefined) {
                                types[currentSelectedType] = 0;
                                numTypes += 1;
                            }
                            types[currentSelectedType]++;
                        }

                        var type = "Multiple";
                        if (numTypes === 1) {
                            type = selections[0].properties.type;
                        }

                        elType.innerHTML = type + " (" + data.selections.length + ")";
                        elTypeIcon.innerHTML = ICON_FOR_TYPE[type];
                        elTypeIcon.style.display = "inline-block";
                        elPropertiesList.className = '';

                        elID.value = "";

                        disableProperties();
                    } else {

                        properties = data.selections[0].properties;
                        if (lastEntityID !== '"' + properties.id + '"' && lastEntityID !== null) {
                            if (editor !== null) {
                                saveJSONUserData(true);
                            }
                            if (materialEditor !== null) {
                                saveJSONMaterialData(true);
                            }
                        }

                        var doSelectElement = lastEntityID === '"' + properties.id + '"';

                        // the event bridge and json parsing handle our avatar id string differently.
                        lastEntityID = '"' + properties.id + '"';
                        elID.value = properties.id;

                        // HTML workaround since image is not yet a separate entity type
                        var IMAGE_MODEL_NAME = 'default-image-model.fbx';
                        if (properties.type === "Model") {
                            var urlParts = properties.modelURL.split('/');
                            var propsFilename = urlParts[urlParts.length - 1];

                            if (propsFilename === IMAGE_MODEL_NAME) {
                                properties.type = "Image";
                            }
                        }

                        // Create class name for css ruleset filtering
                        elPropertiesList.className = properties.type + 'Menu';

                        elType.innerHTML = properties.type;
                        elTypeIcon.innerHTML = ICON_FOR_TYPE[properties.type];
                        elTypeIcon.style.display = "inline-block";

                        elLocked.checked = properties.locked;

                        elName.value = properties.name;

                        elVisible.checked = properties.visible;

                        elPositionX.value = properties.position.x.toFixed(4);
                        elPositionY.value = properties.position.y.toFixed(4);
                        elPositionZ.value = properties.position.z.toFixed(4);

                        elDimensionsX.value = properties.dimensions.x.toFixed(4);
                        elDimensionsY.value = properties.dimensions.y.toFixed(4);
                        elDimensionsZ.value = properties.dimensions.z.toFixed(4);

                        elParentID.value = properties.parentID;
                        elParentJointIndex.value = properties.parentJointIndex;

                        elRegistrationX.value = properties.registrationPoint.x.toFixed(4);
                        elRegistrationY.value = properties.registrationPoint.y.toFixed(4);
                        elRegistrationZ.value = properties.registrationPoint.z.toFixed(4);

                        elRotationX.value = properties.rotation.x.toFixed(4);
                        elRotationY.value = properties.rotation.y.toFixed(4);
                        elRotationZ.value = properties.rotation.z.toFixed(4);

                        elLinearVelocityX.value = properties.velocity.x.toFixed(4);
                        elLinearVelocityY.value = properties.velocity.y.toFixed(4);
                        elLinearVelocityZ.value = properties.velocity.z.toFixed(4);
                        elLinearDamping.value = properties.damping.toFixed(2);

                        elAngularVelocityX.value = (properties.angularVelocity.x * RADIANS_TO_DEGREES).toFixed(4);
                        elAngularVelocityY.value = (properties.angularVelocity.y * RADIANS_TO_DEGREES).toFixed(4);
                        elAngularVelocityZ.value = (properties.angularVelocity.z * RADIANS_TO_DEGREES).toFixed(4);
                        elAngularDamping.value = properties.angularDamping.toFixed(4);

                        elRestitution.value = properties.restitution.toFixed(4);
                        elFriction.value = properties.friction.toFixed(4);

                        elGravityX.value = properties.gravity.x.toFixed(4);
                        elGravityY.value = properties.gravity.y.toFixed(4);
                        elGravityZ.value = properties.gravity.z.toFixed(4);

                        elAccelerationX.value = properties.acceleration.x.toFixed(4);
                        elAccelerationY.value = properties.acceleration.y.toFixed(4);
                        elAccelerationZ.value = properties.acceleration.z.toFixed(4);

                        elDensity.value = properties.density.toFixed(4);
                        elCollisionless.checked = properties.collisionless;
                        elDynamic.checked = properties.dynamic;

                        elCollideStatic.checked = properties.collidesWith.indexOf("static") > -1;
                        elCollideKinematic.checked = properties.collidesWith.indexOf("kinematic") > -1;
                        elCollideDynamic.checked = properties.collidesWith.indexOf("dynamic") > -1;
                        elCollideMyAvatar.checked = properties.collidesWith.indexOf("myAvatar") > -1;
                        elCollideOtherAvatar.checked = properties.collidesWith.indexOf("otherAvatar") > -1;

                        elGrabbable.checked = properties.dynamic;

                        elTriggerable.checked = false;
                        elIgnoreIK.checked = true;

                        elCloneable.checked = properties.cloneable;
                        elCloneableDynamic.checked = properties.cloneDynamic;
                        elCloneableAvatarEntity.checked = properties.cloneAvatarEntity;
                        elCloneableGroup.style.display = elCloneable.checked ? "block": "none";
                        elCloneableLimit.value = properties.cloneLimit;
                        elCloneableLifetime.value = properties.cloneLifetime;

                        var grabbablesSet = false;
                        var parsedUserData = {};
                        try {
                            parsedUserData = JSON.parse(properties.userData);

                            if ("grabbableKey" in parsedUserData) {
                                grabbablesSet = true;
                                var grabbableData = parsedUserData.grabbableKey;
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
                            elCloneable.checked = false;
                        }

                        elCollisionSoundURL.value = properties.collisionSoundURL;
                        elLifetime.value = properties.lifetime;
                        elScriptURL.value = properties.script;
                        elScriptTimestamp.value = properties.scriptTimestamp;
                        elServerScripts.value = properties.serverScripts;

                        var json = null;
                        try {
                            json = JSON.parse(properties.userData);
                        } catch (e) {
                            // normal text
                            deleteJSONEditor();
                            elUserData.value = properties.userData;
                            showUserDataTextArea();
                            showNewJSONEditorButton();
                            hideSaveUserDataButton();
                        }
                        if (json !== null) {
                            if (editor === null) {
                                createJSONEditor();
                            }

                            setEditorJSON(json);
                            showSaveUserDataButton();
                            hideUserDataTextArea();
                            hideNewJSONEditorButton();
                        }

                        var materialJson = null;
                        try {
                            materialJson = JSON.parse(properties.materialData);
                        } catch (e) {
                            // normal text
                            deleteJSONMaterialEditor();
                            elMaterialData.value = properties.materialData;
                            showMaterialDataTextArea();
                            showNewJSONMaterialEditorButton();
                            hideSaveMaterialDataButton();
                        }
                        if (materialJson !== null) {
                            if (materialEditor === null) {
                                createJSONMaterialEditor();
                            }

                            setMaterialEditorJSON(materialJson);
                            showSaveMaterialDataButton();
                            hideMaterialDataTextArea();
                            hideNewJSONMaterialEditorButton();
                        }

                        elHyperlinkHref.value = properties.href;
                        elDescription.value = properties.description;


                        if (properties.type === "Shape" || properties.type === "Box" || properties.type === "Sphere") {
                            elShape.value = properties.shape;
                            setDropdownText(elShape);
                        }

                        if (properties.type === "Shape" || properties.type === "Box" || 
                                properties.type === "Sphere" || properties.type === "ParticleEffect") {
                            elColorRed.value = properties.color.red;
                            elColorGreen.value = properties.color.green;
                            elColorBlue.value = properties.color.blue;
                            elColorControlVariant2.style.backgroundColor = "rgb(" + properties.color.red + "," + 
                                                                     properties.color.green + "," + properties.color.blue + ")";
                        }

                        if (properties.type === "Model" ||
                            properties.type === "Shape" || properties.type === "Box" || properties.type === "Sphere") {

                            elCanCastShadow.checked = properties.canCastShadow;
                        }

                        if (properties.type === "Model") {
                            elModelURL.value = properties.modelURL;
                            elShapeType.value = properties.shapeType;
                            setDropdownText(elShapeType);
                            elCompoundShapeURL.value = properties.compoundShapeURL;
                            elModelAnimationURL.value = properties.animation.url;
                            elModelAnimationPlaying.checked = properties.animation.running;
                            elModelAnimationFPS.value = properties.animation.fps;
                            elModelAnimationFrame.value = properties.animation.currentFrame;
                            elModelAnimationFirstFrame.value = properties.animation.firstFrame;
                            elModelAnimationLastFrame.value = properties.animation.lastFrame;
                            elModelAnimationLoop.checked = properties.animation.loop;
                            elModelAnimationHold.checked = properties.animation.hold;
                            elModelAnimationAllowTranslation.checked = properties.animation.allowTranslation;
                            elModelTextures.value = properties.textures;
                            setTextareaScrolling(elModelTextures);
                            elModelOriginalTextures.value = properties.originalTextures;
                            setTextareaScrolling(elModelOriginalTextures);
                        } else if (properties.type === "Web") {
                            elWebSourceURL.value = properties.sourceUrl;
                            elWebDPI.value = properties.dpi;
                        } else if (properties.type === "Image") {
                            var imageLink = JSON.parse(properties.textures)["tex.picture"];
                            elImageURL.value = imageLink;
                        } else if (properties.type === "Text") {
                            elTextText.value = properties.text;
                            elTextLineHeight.value = properties.lineHeight.toFixed(4);
                            elTextFaceCamera.checked = properties.faceCamera;
                            elTextTextColor.style.backgroundColor = "rgb(" + properties.textColor.red + "," + 
                                                                    properties.textColor.green + "," + 
                                                                    properties.textColor.blue + ")";
                            elTextTextColorRed.value = properties.textColor.red;
                            elTextTextColorGreen.value = properties.textColor.green;
                            elTextTextColorBlue.value = properties.textColor.blue;
                            elTextBackgroundColor.style.backgroundColor = "rgb(" + properties.backgroundColor.red + "," + 
                                                                          properties.backgroundColor.green + "," + 
                                                                          properties.backgroundColor.blue + ")";
                            elTextBackgroundColorRed.value = properties.backgroundColor.red;
                            elTextBackgroundColorGreen.value = properties.backgroundColor.green;
                            elTextBackgroundColorBlue.value = properties.backgroundColor.blue;
                        } else if (properties.type === "Light") {
                            elLightSpotLight.checked = properties.isSpotlight;

                            elLightColor.style.backgroundColor = "rgb(" + properties.color.red + "," + 
                                                                     properties.color.green + "," + properties.color.blue + ")";
                            elLightColorRed.value = properties.color.red;
                            elLightColorGreen.value = properties.color.green;
                            elLightColorBlue.value = properties.color.blue;

                            elLightIntensity.value = properties.intensity.toFixed(1);
                            elLightFalloffRadius.value = properties.falloffRadius.toFixed(1);
                            elLightExponent.value = properties.exponent.toFixed(2);
                            elLightCutoff.value = properties.cutoff.toFixed(2);
                        } else if (properties.type === "Zone") {
                            // Key light
                            elZoneKeyLightModeInherit.checked = (properties.keyLightMode === 'inherit');
                            elZoneKeyLightModeDisabled.checked = (properties.keyLightMode === 'disabled');
                            elZoneKeyLightModeEnabled.checked = (properties.keyLightMode === 'enabled');

                            elZoneKeyLightColor.style.backgroundColor = "rgb(" + properties.keyLight.color.red + "," + 
                                                   properties.keyLight.color.green + "," + properties.keyLight.color.blue + ")";
                            elZoneKeyLightColorRed.value = properties.keyLight.color.red;
                            elZoneKeyLightColorGreen.value = properties.keyLight.color.green;
                            elZoneKeyLightColorBlue.value = properties.keyLight.color.blue;
                            elZoneKeyLightIntensity.value = properties.keyLight.intensity.toFixed(2);
                            elZoneKeyLightDirectionX.value = properties.keyLight.direction.x.toFixed(2);
                            elZoneKeyLightDirectionY.value = properties.keyLight.direction.y.toFixed(2);

                            elZoneKeyLightCastShadows.checked = properties.keyLight.castShadows;

                            // Skybox
                            elZoneSkyboxModeInherit.checked = (properties.skyboxMode === 'inherit');
                            elZoneSkyboxModeDisabled.checked = (properties.skyboxMode === 'disabled');
                            elZoneSkyboxModeEnabled.checked = (properties.skyboxMode === 'enabled');

                            // Ambient light
                            elZoneAmbientLightModeInherit.checked = (properties.ambientLightMode === 'inherit');
                            elZoneAmbientLightModeDisabled.checked = (properties.ambientLightMode === 'disabled');
                            elZoneAmbientLightModeEnabled.checked = (properties.ambientLightMode === 'enabled');

                            elZoneAmbientLightIntensity.value = properties.ambientLight.ambientIntensity.toFixed(2);
                            elZoneAmbientLightURL.value = properties.ambientLight.ambientURL;

                            // Haze
                            elZoneHazeModeInherit.checked = (properties.hazeMode === 'inherit');
                            elZoneHazeModeDisabled.checked = (properties.hazeMode === 'disabled');
                            elZoneHazeModeEnabled.checked = (properties.hazeMode === 'enabled');

                            elZoneHazeRange.value = properties.haze.hazeRange.toFixed(0);
                            elZoneHazeColor.style.backgroundColor = "rgb(" + 
                                properties.haze.hazeColor.red + "," + 
                                properties.haze.hazeColor.green + "," + 
                                properties.haze.hazeColor.blue + ")";

                            elZoneHazeColorRed.value = properties.haze.hazeColor.red;
                            elZoneHazeColorGreen.value = properties.haze.hazeColor.green;
                            elZoneHazeColorBlue.value = properties.haze.hazeColor.blue;
                            elZoneHazeBackgroundBlend.value = properties.haze.hazeBackgroundBlend.toFixed(2);

                            elZoneHazeGlareColor.style.backgroundColor = "rgb(" + 
                                properties.haze.hazeGlareColor.red + "," + 
                                properties.haze.hazeGlareColor.green + "," + 
                                properties.haze.hazeGlareColor.blue + ")";

                            elZoneHazeGlareColorRed.value = properties.haze.hazeGlareColor.red;
                            elZoneHazeGlareColorGreen.value = properties.haze.hazeGlareColor.green;
                            elZoneHazeGlareColorBlue.value = properties.haze.hazeGlareColor.blue;

                            elZoneHazeEnableGlare.checked = properties.haze.hazeEnableGlare;
                            elZoneHazeGlareAngle.value = properties.haze.hazeGlareAngle.toFixed(0);

                            elZoneHazeAltitudeEffect.checked = properties.haze.hazeAltitudeEffect;
                            elZoneHazeBaseRef.value = properties.haze.hazeBaseRef.toFixed(0);
                            elZoneHazeCeiling.value = properties.haze.hazeCeiling.toFixed(0);

                            elZoneBloomModeInherit.checked = (properties.bloomMode === 'inherit');
                            elZoneBloomModeDisabled.checked = (properties.bloomMode === 'disabled');
                            elZoneBloomModeEnabled.checked = (properties.bloomMode === 'enabled');

                            elZoneBloomIntensity.value = properties.bloom.bloomIntensity.toFixed(2);
                            elZoneBloomThreshold.value = properties.bloom.bloomThreshold.toFixed(2);
                            elZoneBloomSize.value = properties.bloom.bloomSize.toFixed(2);

                            elShapeType.value = properties.shapeType;
                            elCompoundShapeURL.value = properties.compoundShapeURL;

                            elZoneSkyboxColor.style.backgroundColor = "rgb(" + properties.skybox.color.red + "," + 
                                                       properties.skybox.color.green + "," + properties.skybox.color.blue + ")";
                            elZoneSkyboxColorRed.value = properties.skybox.color.red;
                            elZoneSkyboxColorGreen.value = properties.skybox.color.green;
                            elZoneSkyboxColorBlue.value = properties.skybox.color.blue;
                            elZoneSkyboxURL.value = properties.skybox.url;

                            elZoneFlyingAllowed.checked = properties.flyingAllowed;
                            elZoneGhostingAllowed.checked = properties.ghostingAllowed;
                            elZoneFilterURL.value = properties.filterURL;

                            // Show/hide sections as required
                            showElements(document.getElementsByClassName('skybox-section'),
                                elZoneSkyboxModeEnabled.checked);

                            showElements(document.getElementsByClassName('keylight-section'),
                                elZoneKeyLightModeEnabled.checked);

                            showElements(document.getElementsByClassName('ambient-section'),
                                elZoneAmbientLightModeEnabled.checked);

                            showElements(document.getElementsByClassName('haze-section'),
                                elZoneHazeModeEnabled.checked);

                            showElements(document.getElementsByClassName('bloom-section'),
                                elZoneBloomModeEnabled.checked);
                        } else if (properties.type === "PolyVox") {
                            elVoxelVolumeSizeX.value = properties.voxelVolumeSize.x.toFixed(2);
                            elVoxelVolumeSizeY.value = properties.voxelVolumeSize.y.toFixed(2);
                            elVoxelVolumeSizeZ.value = properties.voxelVolumeSize.z.toFixed(2);
                            elVoxelSurfaceStyle.value = properties.voxelSurfaceStyle;
                            setDropdownText(elVoxelSurfaceStyle);
                            elXTextureURL.value = properties.xTextureURL;
                            elYTextureURL.value = properties.yTextureURL;
                            elZTextureURL.value = properties.zTextureURL;
                        } else if (properties.type === "Material") {
                            elMaterialURL.value = properties.materialURL;
                            //elMaterialMappingMode.value = properties.materialMappingMode;
                            //setDropdownText(elMaterialMappingMode);
                            elPriority.value = properties.priority;
                            if (properties.parentMaterialName.startsWith(MATERIAL_PREFIX_STRING)) {
                                elParentMaterialNameString.value = properties.parentMaterialName.replace(MATERIAL_PREFIX_STRING, "");
                                showParentMaterialNameBox(false, elParentMaterialNameNumber, elParentMaterialNameString);
                                elParentMaterialNameCheckbox.checked = false;
                            } else {
                                elParentMaterialNameNumber.value = parseInt(properties.parentMaterialName);
                                showParentMaterialNameBox(true, elParentMaterialNameNumber, elParentMaterialNameString);
                                elParentMaterialNameCheckbox.checked = true;
                            }
                            elMaterialMappingPosX.value = properties.materialMappingPos.x.toFixed(4);
                            elMaterialMappingPosY.value = properties.materialMappingPos.y.toFixed(4);
                            elMaterialMappingScaleX.value = properties.materialMappingScale.x.toFixed(4);
                            elMaterialMappingScaleY.value = properties.materialMappingScale.y.toFixed(4);
                            elMaterialMappingRot.value = properties.materialMappingRot.toFixed(2);
                        }

                        // Only these types can cast a shadow
                        if (properties.type === "Model" ||
                            properties.type === "Shape" || properties.type === "Box" || properties.type === "Sphere") {

                            showElements(document.getElementsByClassName('can-cast-shadow-section'), true);
                        } else {
                            showElements(document.getElementsByClassName('can-cast-shadow-section'), false);
                        }

                        if (properties.locked) {
                            disableProperties();
                            elLocked.removeAttribute('disabled');
                        } else {
                            enableProperties();
                            elSaveUserData.disabled = true;
                            elSaveMaterialData.disabled = true;
                        }

                        var activeElement = document.activeElement;
                        if (doSelectElement && typeof activeElement.select !== "undefined") {
                            activeElement.select();
                        }
                    }
                }
            });
        }

        elLocked.addEventListener('change', createEmitCheckedPropertyUpdateFunction('locked'));
        elName.addEventListener('change', createEmitTextPropertyUpdateFunction('name'));
        elHyperlinkHref.addEventListener('change', createEmitTextPropertyUpdateFunction('href'));
        elDescription.addEventListener('change', createEmitTextPropertyUpdateFunction('description'));
        elVisible.addEventListener('change', createEmitCheckedPropertyUpdateFunction('visible'));

        var positionChangeFunction = createEmitVec3PropertyUpdateFunction(
            'position', elPositionX, elPositionY, elPositionZ);
        elPositionX.addEventListener('change', positionChangeFunction);
        elPositionY.addEventListener('change', positionChangeFunction);
        elPositionZ.addEventListener('change', positionChangeFunction);

        var dimensionsChangeFunction = createEmitVec3PropertyUpdateFunction(
            'dimensions', elDimensionsX, elDimensionsY, elDimensionsZ);
        elDimensionsX.addEventListener('change', dimensionsChangeFunction);
        elDimensionsY.addEventListener('change', dimensionsChangeFunction);
        elDimensionsZ.addEventListener('change', dimensionsChangeFunction);

        elParentID.addEventListener('change', createEmitTextPropertyUpdateFunction('parentID'));
        elParentJointIndex.addEventListener('change', createEmitNumberPropertyUpdateFunction('parentJointIndex', 0));

        var registrationChangeFunction = createEmitVec3PropertyUpdateFunction(
            'registrationPoint', elRegistrationX, elRegistrationY, elRegistrationZ);
        elRegistrationX.addEventListener('change', registrationChangeFunction);
        elRegistrationY.addEventListener('change', registrationChangeFunction);
        elRegistrationZ.addEventListener('change', registrationChangeFunction);

        var rotationChangeFunction = createEmitVec3PropertyUpdateFunction(
            'rotation', elRotationX, elRotationY, elRotationZ);
        elRotationX.addEventListener('change', rotationChangeFunction);
        elRotationY.addEventListener('change', rotationChangeFunction);
        elRotationZ.addEventListener('change', rotationChangeFunction);

        var velocityChangeFunction = createEmitVec3PropertyUpdateFunction(
            'velocity', elLinearVelocityX, elLinearVelocityY, elLinearVelocityZ);
        elLinearVelocityX.addEventListener('change', velocityChangeFunction);
        elLinearVelocityY.addEventListener('change', velocityChangeFunction);
        elLinearVelocityZ.addEventListener('change', velocityChangeFunction);
        elLinearDamping.addEventListener('change', createEmitNumberPropertyUpdateFunction('damping'));

        var angularVelocityChangeFunction = createEmitVec3PropertyUpdateFunctionWithMultiplier(
            'angularVelocity', elAngularVelocityX, elAngularVelocityY, elAngularVelocityZ, DEGREES_TO_RADIANS);
        elAngularVelocityX.addEventListener('change', angularVelocityChangeFunction);
        elAngularVelocityY.addEventListener('change', angularVelocityChangeFunction);
        elAngularVelocityZ.addEventListener('change', angularVelocityChangeFunction);
        elAngularDamping.addEventListener('change', createEmitNumberPropertyUpdateFunction('angularDamping'));

        elRestitution.addEventListener('change', createEmitNumberPropertyUpdateFunction('restitution'));
        elFriction.addEventListener('change', createEmitNumberPropertyUpdateFunction('friction'));

        var gravityChangeFunction = createEmitVec3PropertyUpdateFunction(
            'gravity', elGravityX, elGravityY, elGravityZ);
        elGravityX.addEventListener('change', gravityChangeFunction);
        elGravityY.addEventListener('change', gravityChangeFunction);
        elGravityZ.addEventListener('change', gravityChangeFunction);

        var accelerationChangeFunction = createEmitVec3PropertyUpdateFunction(
            'acceleration', elAccelerationX, elAccelerationY, elAccelerationZ);
        elAccelerationX.addEventListener('change', accelerationChangeFunction);
        elAccelerationY.addEventListener('change', accelerationChangeFunction);
        elAccelerationZ.addEventListener('change', accelerationChangeFunction);

        elDensity.addEventListener('change', createEmitNumberPropertyUpdateFunction('density'));
        elCollisionless.addEventListener('change', createEmitCheckedPropertyUpdateFunction('collisionless'));
        elDynamic.addEventListener('change', createEmitCheckedPropertyUpdateFunction('dynamic'));

        elCollideDynamic.addEventListener('change', function() {
            updateCheckedSubProperty("collidesWith", properties.collidesWith, elCollideDynamic, 'dynamic');
        });

        elCollideKinematic.addEventListener('change', function() {
            updateCheckedSubProperty("collidesWith", properties.collidesWith, elCollideKinematic, 'kinematic');
        });

        elCollideStatic.addEventListener('change', function() {
            updateCheckedSubProperty("collidesWith", properties.collidesWith, elCollideStatic, 'static');
        });
        elCollideMyAvatar.addEventListener('change', function() {
            updateCheckedSubProperty("collidesWith", properties.collidesWith, elCollideMyAvatar, 'myAvatar');
        });
        elCollideOtherAvatar.addEventListener('change', function() {
            updateCheckedSubProperty("collidesWith", properties.collidesWith, elCollideOtherAvatar, 'otherAvatar');
        });

        elGrabbable.addEventListener('change', function() {
            if (elCloneable.checked) {
                elGrabbable.checked = false;
            }
            userDataChanger("grabbableKey", "grabbable", elGrabbable, elUserData, true);
        });
        
        elCloneable.addEventListener('change', createEmitCheckedPropertyUpdateFunction('cloneable'));
        elCloneableDynamic.addEventListener('change', createEmitCheckedPropertyUpdateFunction('cloneDynamic'));
        elCloneableAvatarEntity.addEventListener('change', createEmitCheckedPropertyUpdateFunction('cloneAvatarEntity'));
        elCloneableLifetime.addEventListener('change', createEmitNumberPropertyUpdateFunction('cloneLifetime'));
        elCloneableLimit.addEventListener('change', createEmitNumberPropertyUpdateFunction('cloneLimit'));

        elTriggerable.addEventListener('change', function() {
            userDataChanger("grabbableKey", "triggerable", elTriggerable, elUserData, false, ['wantsTrigger']);
        });
        elIgnoreIK.addEventListener('change', function() {
            userDataChanger("grabbableKey", "ignoreIK", elIgnoreIK, elUserData, true);
        });

        elCollisionSoundURL.addEventListener('change', createEmitTextPropertyUpdateFunction('collisionSoundURL'));

        elLifetime.addEventListener('change', createEmitNumberPropertyUpdateFunction('lifetime'));
        elScriptURL.addEventListener('change', createEmitTextPropertyUpdateFunction('script'));
        elScriptTimestamp.addEventListener('change', createEmitNumberPropertyUpdateFunction('scriptTimestamp'));
        elServerScripts.addEventListener('change', createEmitTextPropertyUpdateFunction('serverScripts'));
        elServerScripts.addEventListener('change', function() {
            // invalidate the current status (so that same-same updates can still be observed visually)
            elServerScriptStatus.innerText = PENDING_SCRIPT_STATUS;
        });

        elClearUserData.addEventListener("click", function() {
            deleteJSONEditor();
            elUserData.value = "";
            showUserDataTextArea();
            showNewJSONEditorButton();
            hideSaveUserDataButton();
            updateProperty('userData', elUserData.value);
        });

        elSaveUserData.addEventListener("click", function() {
            saveJSONUserData(true);
        });

        elUserData.addEventListener('change', createEmitTextPropertyUpdateFunction('userData'));

        elNewJSONEditor.addEventListener('click', function() {
            deleteJSONEditor();
            createJSONEditor();
            var data = {};
            setEditorJSON(data);
            hideUserDataTextArea();
            hideNewJSONEditorButton();
            showSaveUserDataButton();
        });

        elClearMaterialData.addEventListener("click", function() {
            deleteJSONMaterialEditor();
            elMaterialData.value = "";
            showMaterialDataTextArea();
            showNewJSONMaterialEditorButton();
            hideSaveMaterialDataButton();
            updateProperty('materialData', elMaterialData.value);
        });

        elSaveMaterialData.addEventListener("click", function() {
            saveJSONMaterialData(true);
        });

        elMaterialData.addEventListener('change', createEmitTextPropertyUpdateFunction('materialData'));

        elNewJSONMaterialEditor.addEventListener('click', function() {
            deleteJSONMaterialEditor();
            createJSONMaterialEditor();
            var data = {};
            setMaterialEditorJSON(data);
            hideMaterialDataTextArea();
            hideNewJSONMaterialEditorButton();
            showSaveMaterialDataButton();
        });

        var colorChangeFunction = createEmitColorPropertyUpdateFunction(
            'color', elColorRed, elColorGreen, elColorBlue);
        elColorRed.addEventListener('change', colorChangeFunction);
        elColorGreen.addEventListener('change', colorChangeFunction);
        elColorBlue.addEventListener('change', colorChangeFunction);
        colorPickers['#property-color-control2'] = $('#property-color-control2').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            submit: false, // We don't want to have a submission button
            onShow: function(colpick) {
                $('#property-color-control2').attr('active', 'true');
                // The original color preview within the picker needs to be updated on show because
                // prior to the picker being shown we don't have access to the selections' starting color.
                colorPickers['#property-color-control2'].colpickSetColor({ 
                    "r": elColorRed.value, 
                    "g": elColorGreen.value, 
                    "b": elColorBlue.value});
            },
            onHide: function(colpick) {
                $('#property-color-control2').attr('active', 'false');
            },
            onChange: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                emitColorPropertyUpdate('color', rgb.r, rgb.g, rgb.b);
            }
        });

        elLightSpotLight.addEventListener('change', createEmitCheckedPropertyUpdateFunction('isSpotlight'));

        var lightColorChangeFunction = createEmitColorPropertyUpdateFunction(
            'color', elLightColorRed, elLightColorGreen, elLightColorBlue);
        elLightColorRed.addEventListener('change', lightColorChangeFunction);
        elLightColorGreen.addEventListener('change', lightColorChangeFunction);
        elLightColorBlue.addEventListener('change', lightColorChangeFunction);
        colorPickers['#property-light-color'] = $('#property-light-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            submit: false, // We don't want to have a submission button
            onShow: function(colpick) {
                $('#property-light-color').attr('active', 'true');
                // The original color preview within the picker needs to be updated on show because
                // prior to the picker being shown we don't have access to the selections' starting color.
                colorPickers['#property-light-color'].colpickSetColor({
                    "r": elLightColorRed.value,
                    "g": elLightColorGreen.value,
                    "b": elLightColorBlue.value
                });
            },
            onHide: function(colpick) {
                $('#property-light-color').attr('active', 'false');
            },
            onChange: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                emitColorPropertyUpdate('color', rgb.r, rgb.g, rgb.b);
            }
        });

        elLightIntensity.addEventListener('change', createEmitNumberPropertyUpdateFunction('intensity', 1));
        elLightFalloffRadius.addEventListener('change', createEmitNumberPropertyUpdateFunction('falloffRadius', 1));
        elLightExponent.addEventListener('change', createEmitNumberPropertyUpdateFunction('exponent', 2));
        elLightCutoff.addEventListener('change', createEmitNumberPropertyUpdateFunction('cutoff', 2));

        elShape.addEventListener('change', createEmitTextPropertyUpdateFunction('shape'));

        elCanCastShadow.addEventListener('change', createEmitCheckedPropertyUpdateFunction('canCastShadow'));
 
        elImageURL.addEventListener('change', createImageURLUpdateFunction('textures'));

        elWebSourceURL.addEventListener('change', createEmitTextPropertyUpdateFunction('sourceUrl'));
        elWebDPI.addEventListener('change', createEmitNumberPropertyUpdateFunction('dpi', 0));

        elModelURL.addEventListener('change', createEmitTextPropertyUpdateFunction('modelURL'));
        elShapeType.addEventListener('change', createEmitTextPropertyUpdateFunction('shapeType'));
        elCompoundShapeURL.addEventListener('change', createEmitTextPropertyUpdateFunction('compoundShapeURL'));

        elModelAnimationURL.addEventListener('change', createEmitGroupTextPropertyUpdateFunction('animation', 'url'));
        elModelAnimationPlaying.addEventListener('change',createEmitGroupCheckedPropertyUpdateFunction('animation', 'running'));
        elModelAnimationFPS.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('animation', 'fps'));
        elModelAnimationFrame.addEventListener('change', 
            createEmitGroupNumberPropertyUpdateFunction('animation', 'currentFrame'));
        elModelAnimationFirstFrame.addEventListener('change', 
            createEmitGroupNumberPropertyUpdateFunction('animation', 'firstFrame'));
        elModelAnimationLastFrame.addEventListener('change', 
            createEmitGroupNumberPropertyUpdateFunction('animation', 'lastFrame'));
        elModelAnimationLoop.addEventListener('change', createEmitGroupCheckedPropertyUpdateFunction('animation', 'loop'));
        elModelAnimationHold.addEventListener('change', createEmitGroupCheckedPropertyUpdateFunction('animation', 'hold'));
        elModelAnimationAllowTranslation.addEventListener('change', 
            createEmitGroupCheckedPropertyUpdateFunction('animation', 'allowTranslation'));

        elModelTextures.addEventListener('change', createEmitTextPropertyUpdateFunction('textures'));

        elMaterialURL.addEventListener('change', createEmitTextPropertyUpdateFunction('materialURL'));
        //elMaterialMappingMode.addEventListener('change', createEmitTextPropertyUpdateFunction('materialMappingMode'));
        elPriority.addEventListener('change', createEmitNumberPropertyUpdateFunction('priority', 0));

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

        var materialMappingPosChangeFunction = createEmitVec2PropertyUpdateFunction('materialMappingPos', elMaterialMappingPosX, elMaterialMappingPosY);
        elMaterialMappingPosX.addEventListener('change', materialMappingPosChangeFunction);
        elMaterialMappingPosY.addEventListener('change', materialMappingPosChangeFunction);
        var materialMappingScaleChangeFunction = createEmitVec2PropertyUpdateFunction('materialMappingScale', elMaterialMappingScaleX, elMaterialMappingScaleY);
        elMaterialMappingScaleX.addEventListener('change', materialMappingScaleChangeFunction);
        elMaterialMappingScaleY.addEventListener('change', materialMappingScaleChangeFunction);
        elMaterialMappingRot.addEventListener('change', createEmitNumberPropertyUpdateFunction('materialMappingRot', 2));

        elTextText.addEventListener('change', createEmitTextPropertyUpdateFunction('text'));
        elTextFaceCamera.addEventListener('change', createEmitCheckedPropertyUpdateFunction('faceCamera'));
        elTextLineHeight.addEventListener('change', createEmitNumberPropertyUpdateFunction('lineHeight'));
        var textTextColorChangeFunction = createEmitColorPropertyUpdateFunction(
            'textColor', elTextTextColorRed, elTextTextColorGreen, elTextTextColorBlue);
        elTextTextColorRed.addEventListener('change', textTextColorChangeFunction);
        elTextTextColorGreen.addEventListener('change', textTextColorChangeFunction);
        elTextTextColorBlue.addEventListener('change', textTextColorChangeFunction);
        colorPickers['#property-text-text-color'] = $('#property-text-text-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            submit: false, // We don't want to have a submission button
            onShow: function(colpick) {
                $('#property-text-text-color').attr('active', 'true');
                // The original color preview within the picker needs to be updated on show because
                // prior to the picker being shown we don't have access to the selections' starting color.
                colorPickers['#property-text-text-color'].colpickSetColor({
                    "r": elTextTextColorRed.value,
                    "g": elTextTextColorGreen.value,
                    "b": elTextTextColorBlue.value
                });
            },
            onHide: function(colpick) {
                $('#property-text-text-color').attr('active', 'false');
            },
            onChange: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                $(el).attr('active', 'false');
                emitColorPropertyUpdate('textColor', rgb.r, rgb.g, rgb.b);
            }
        });

        var textBackgroundColorChangeFunction = createEmitColorPropertyUpdateFunction(
            'backgroundColor', elTextBackgroundColorRed, elTextBackgroundColorGreen, elTextBackgroundColorBlue);

        elTextBackgroundColorRed.addEventListener('change', textBackgroundColorChangeFunction);
        elTextBackgroundColorGreen.addEventListener('change', textBackgroundColorChangeFunction);
        elTextBackgroundColorBlue.addEventListener('change', textBackgroundColorChangeFunction);
        colorPickers['#property-text-background-color'] = $('#property-text-background-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            submit: false, // We don't want to have a submission button
            onShow: function(colpick) {
                $('#property-text-background-color').attr('active', 'true');
                // The original color preview within the picker needs to be updated on show because
                // prior to the picker being shown we don't have access to the selections' starting color.
                colorPickers['#property-text-background-color'].colpickSetColor({
                    "r": elTextBackgroundColorRed.value,
                    "g": elTextBackgroundColorGreen.value,
                    "b": elTextBackgroundColorBlue.value
                });
            },
            onHide: function(colpick) {
                $('#property-text-background-color').attr('active', 'false');
            },
            onChange: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                emitColorPropertyUpdate('backgroundColor', rgb.r, rgb.g, rgb.b);
            }
        });

        // Key light
        var keyLightModeChanged = createZoneComponentModeChangedFunction('keyLightMode',
            elZoneKeyLightModeInherit, elZoneKeyLightModeDisabled, elZoneKeyLightModeEnabled);

        elZoneKeyLightModeInherit.addEventListener('change', keyLightModeChanged);
        elZoneKeyLightModeDisabled.addEventListener('change', keyLightModeChanged);
        elZoneKeyLightModeEnabled.addEventListener('change', keyLightModeChanged);

        colorPickers['#property-zone-key-light-color'] = $('#property-zone-key-light-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            submit: false, // We don't want to have a submission button
            onShow: function(colpick) {
                $('#property-zone-key-light-color').attr('active', 'true');
                // The original color preview within the picker needs to be updated on show because
                // prior to the picker being shown we don't have access to the selections' starting color.
                colorPickers['#property-zone-key-light-color'].colpickSetColor({
                    "r": elZoneKeyLightColorRed.value,
                    "g": elZoneKeyLightColorGreen.value,
                    "b": elZoneKeyLightColorBlue.value
                });
            },
            onHide: function(colpick) {
                $('#property-zone-key-light-color').attr('active', 'false');
            },
            onChange: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                emitColorPropertyUpdate('color', rgb.r, rgb.g, rgb.b, 'keyLight');
            }
        });
        var zoneKeyLightColorChangeFunction = createEmitGroupColorPropertyUpdateFunction('keyLight', 'color', 
            elZoneKeyLightColorRed, elZoneKeyLightColorGreen, elZoneKeyLightColorBlue);

        elZoneKeyLightColorRed.addEventListener('change', zoneKeyLightColorChangeFunction);
        elZoneKeyLightColorGreen.addEventListener('change', zoneKeyLightColorChangeFunction);
        elZoneKeyLightColorBlue.addEventListener('change', zoneKeyLightColorChangeFunction);
        elZoneKeyLightIntensity.addEventListener('change', 
            createEmitGroupNumberPropertyUpdateFunction('keyLight', 'intensity'));

        var zoneKeyLightDirectionChangeFunction = createEmitGroupVec3PropertyUpdateFunction('keyLight', 'direction', 
            elZoneKeyLightDirectionX, elZoneKeyLightDirectionY);

        elZoneKeyLightDirectionX.addEventListener('change', zoneKeyLightDirectionChangeFunction);
        elZoneKeyLightDirectionY.addEventListener('change', zoneKeyLightDirectionChangeFunction);

        elZoneKeyLightCastShadows.addEventListener('change',
            createEmitGroupCheckedPropertyUpdateFunction('keyLight', 'castShadows'));

        // Skybox
        var skyboxModeChanged = createZoneComponentModeChangedFunction('skyboxMode',
            elZoneSkyboxModeInherit, elZoneSkyboxModeDisabled, elZoneSkyboxModeEnabled);

        elZoneSkyboxModeInherit.addEventListener('change', skyboxModeChanged);
        elZoneSkyboxModeDisabled.addEventListener('change', skyboxModeChanged);
        elZoneSkyboxModeEnabled.addEventListener('change', skyboxModeChanged);

        // Ambient light
        elCopySkyboxURLToAmbientURL.addEventListener("click", function () {
            document.getElementById("property-zone-key-ambient-url").value = properties.skybox.url;
            properties.ambientLight.ambientURL = properties.skybox.url;
            updateProperties(properties);
        });

        var ambientLightModeChanged = createZoneComponentModeChangedFunction('ambientLightMode',
            elZoneAmbientLightModeInherit, elZoneAmbientLightModeDisabled, elZoneAmbientLightModeEnabled);

        elZoneAmbientLightModeInherit.addEventListener('change', ambientLightModeChanged);
        elZoneAmbientLightModeDisabled.addEventListener('change', ambientLightModeChanged);
        elZoneAmbientLightModeEnabled.addEventListener('change', ambientLightModeChanged);

        elZoneAmbientLightIntensity.addEventListener('change',
            createEmitGroupNumberPropertyUpdateFunction('ambientLight', 'ambientIntensity'));

        elZoneAmbientLightURL.addEventListener('change',
            createEmitGroupTextPropertyUpdateFunction('ambientLight', 'ambientURL'));

        // Haze
        var hazeModeChanged = createZoneComponentModeChangedFunction('hazeMode',
            elZoneHazeModeInherit, elZoneHazeModeDisabled, elZoneHazeModeEnabled);

        elZoneHazeModeInherit.addEventListener('change', hazeModeChanged);
        elZoneHazeModeDisabled.addEventListener('change', hazeModeChanged);
        elZoneHazeModeEnabled.addEventListener('change', hazeModeChanged);

        elZoneHazeRange.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('haze', 'hazeRange'));

        colorPickers['#property-zone-haze-color'] = $('#property-zone-haze-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            submit: false, // We don't want to have a submission button
            onShow: function(colpick) {
                $('#property-zone-haze-color').attr('active', 'true');
                // The original color preview within the picker needs to be updated on show because
                // prior to the picker being shown we don't have access to the selections' starting color.
                colorPickers['#property-zone-haze-color'].colpickSetColor({
                    "r": elZoneHazeColorRed.value,
                    "g": elZoneHazeColorGreen.value,
                    "b": elZoneHazeColorBlue.value
                });
            },
            onHide: function(colpick) {
                $('#property-zone-haze-color').attr('active', 'false');
            },
            onChange: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                emitColorPropertyUpdate('hazeColor', rgb.r, rgb.g, rgb.b, 'haze');
            }
        });
        var zoneHazeColorChangeFunction = createEmitGroupColorPropertyUpdateFunction('haze', 'hazeColor', 
            elZoneHazeColorRed, 
            elZoneHazeColorGreen, 
            elZoneHazeColorBlue);

        elZoneHazeColorRed.addEventListener('change', zoneHazeColorChangeFunction);
        elZoneHazeColorGreen.addEventListener('change', zoneHazeColorChangeFunction);
        elZoneHazeColorBlue.addEventListener('change', zoneHazeColorChangeFunction);

        colorPickers['#property-zone-haze-glare-color'] = $('#property-zone-haze-glare-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            submit: false, // We don't want to have a submission button
            onShow: function(colpick) {
                $('#property-zone-haze-glare-color').attr('active', 'true');
                // The original color preview within the picker needs to be updated on show because
                // prior to the picker being shown we don't have access to the selections' starting color.
                colorPickers['#property-zone-haze-glare-color'].colpickSetColor({
                    "r": elZoneHazeGlareColorRed.value,
                    "g": elZoneHazeGlareColorGreen.value,
                    "b": elZoneHazeGlareColorBlue.value
                });
            },
            onHide: function(colpick) {
                $('#property-zone-haze-glare-color').attr('active', 'false');
            },
            onChange: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                emitColorPropertyUpdate('hazeGlareColor', rgb.r, rgb.g, rgb.b, 'haze');
            }
        });
        var zoneHazeGlareColorChangeFunction = createEmitGroupColorPropertyUpdateFunction('haze', 'hazeGlareColor', 
            elZoneHazeGlareColorRed, 
            elZoneHazeGlareColorGreen, 
            elZoneHazeGlareColorBlue);

        elZoneHazeGlareColorRed.addEventListener('change', zoneHazeGlareColorChangeFunction);
        elZoneHazeGlareColorGreen.addEventListener('change', zoneHazeGlareColorChangeFunction);
        elZoneHazeGlareColorBlue.addEventListener('change', zoneHazeGlareColorChangeFunction);

        elZoneHazeEnableGlare.addEventListener('change', 
            createEmitGroupCheckedPropertyUpdateFunction('haze', 'hazeEnableGlare'));
        elZoneHazeGlareAngle.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('haze', 'hazeGlareAngle'));

        elZoneHazeAltitudeEffect.addEventListener('change', 
            createEmitGroupCheckedPropertyUpdateFunction('haze', 'hazeAltitudeEffect'));
        elZoneHazeCeiling.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('haze', 'hazeCeiling'));
        elZoneHazeBaseRef.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('haze', 'hazeBaseRef'));

        elZoneHazeBackgroundBlend.addEventListener('change', 
            createEmitGroupNumberPropertyUpdateFunction('haze', 'hazeBackgroundBlend'));

        // Bloom
        var bloomModeChanged = createZoneComponentModeChangedFunction('bloomMode',
            elZoneBloomModeInherit, elZoneBloomModeDisabled, elZoneBloomModeEnabled);

        elZoneBloomModeInherit.addEventListener('change', bloomModeChanged);
        elZoneBloomModeDisabled.addEventListener('change', bloomModeChanged);
        elZoneBloomModeEnabled.addEventListener('change', bloomModeChanged);

        elZoneBloomIntensity.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('bloom', 'bloomIntensity'));
        elZoneBloomThreshold.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('bloom', 'bloomThreshold'));
        elZoneBloomSize.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('bloom', 'bloomSize'));

        var zoneSkyboxColorChangeFunction = createEmitGroupColorPropertyUpdateFunction('skybox', 'color',
            elZoneSkyboxColorRed, elZoneSkyboxColorGreen, elZoneSkyboxColorBlue);
        elZoneSkyboxColorRed.addEventListener('change', zoneSkyboxColorChangeFunction);
        elZoneSkyboxColorGreen.addEventListener('change', zoneSkyboxColorChangeFunction);
        elZoneSkyboxColorBlue.addEventListener('change', zoneSkyboxColorChangeFunction);
        colorPickers['#property-zone-skybox-color'] = $('#property-zone-skybox-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            submit: false, // We don't want to have a submission button
            onShow: function(colpick) {
                $('#property-zone-skybox-color').attr('active', 'true');
                // The original color preview within the picker needs to be updated on show because
                // prior to the picker being shown we don't have access to the selections' starting color.
                colorPickers['#property-zone-skybox-color'].colpickSetColor({
                    "r": elZoneSkyboxColorRed.value,
                    "g": elZoneSkyboxColorGreen.value,
                    "b": elZoneSkyboxColorBlue.value
                });
            },
            onHide: function(colpick) {
                $('#property-zone-skybox-color').attr('active', 'false');
            },
            onChange: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                emitColorPropertyUpdate('color', rgb.r, rgb.g, rgb.b, 'skybox');
            }
        });

        elZoneSkyboxURL.addEventListener('change', createEmitGroupTextPropertyUpdateFunction('skybox', 'url'));

        elZoneFlyingAllowed.addEventListener('change', createEmitCheckedPropertyUpdateFunction('flyingAllowed'));
        elZoneGhostingAllowed.addEventListener('change', createEmitCheckedPropertyUpdateFunction('ghostingAllowed'));
        elZoneFilterURL.addEventListener('change', createEmitTextPropertyUpdateFunction('filterURL'));

        var voxelVolumeSizeChangeFunction = createEmitVec3PropertyUpdateFunction(
            'voxelVolumeSize', elVoxelVolumeSizeX, elVoxelVolumeSizeY, elVoxelVolumeSizeZ);
        elVoxelVolumeSizeX.addEventListener('change', voxelVolumeSizeChangeFunction);
        elVoxelVolumeSizeY.addEventListener('change', voxelVolumeSizeChangeFunction);
        elVoxelVolumeSizeZ.addEventListener('change', voxelVolumeSizeChangeFunction);
        elVoxelSurfaceStyle.addEventListener('change', createEmitTextPropertyUpdateFunction('voxelSurfaceStyle'));
        elXTextureURL.addEventListener('change', createEmitTextPropertyUpdateFunction('xTextureURL'));
        elYTextureURL.addEventListener('change', createEmitTextPropertyUpdateFunction('yTextureURL'));
        elZTextureURL.addEventListener('change', createEmitTextPropertyUpdateFunction('zTextureURL'));

        elMoveSelectionToGrid.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "moveSelectionToGrid"
            }));
        });
        elMoveAllToGrid.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "moveAllToGrid"
            }));
        });
        elResetToNaturalDimensions.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "resetToNaturalDimensions"
            }));
        });
        elRescaleDimensionsButton.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "rescaleDimensions",
                percentage: parseFloat(elRescaleDimensionsPct.value)
            }));
        });
        elReloadScriptsButton.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "reloadClientScripts"
            }));
        });
        elReloadServerScriptsButton.addEventListener("click", function() {
            // invalidate the current status (so that same-same updates can still be observed visually)
            elServerScriptStatus.innerText = PENDING_SCRIPT_STATUS;
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "reloadServerScripts"
            }));
        });

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
            var ev = document.createEvent("HTMLEvents");
            ev.initEvent("change", true, true);
            document.activeElement.dispatchEvent(ev);
        };

        // For input and textarea elements, select all of the text on focus
        var els = document.querySelectorAll("input, textarea");
        for (var i = 0; i < els.length; i++) {
            els[i].onfocus = function (e) {
                e.target.select();
            };
        }

        bindAllNonJSONEditorElements();
    });

    // Collapsible sections
    var elCollapsible = document.getElementsByClassName("section-header");

    var toggleCollapsedEvent = function(event) {
        var element = event.target.parentNode.parentNode;
        var isCollapsed = element.dataset.collapsed !== "true";
        element.dataset.collapsed = isCollapsed ? "true" : false;
        element.setAttribute("collapsed", isCollapsed ? "true" : "false");
        element.getElementsByClassName(".collapse-icon")[0].textContent = isCollapsed ? "L" : "M";
    };

    for (var collapseIndex = 0, numCollapsibles = elCollapsible.length; collapseIndex < numCollapsibles; ++collapseIndex) {
        var curCollapsibleElement = elCollapsible[collapseIndex];
        curCollapsibleElement.getElementsByTagName('span')[0].addEventListener("click", toggleCollapsedEvent, true);
    }


    // Textarea scrollbars
    var elTextareas = document.getElementsByTagName("TEXTAREA");

    var textareaOnChangeEvent = function(event) {
        setTextareaScrolling(event.target);
    };

    for (var textAreaIndex = 0, numTextAreas = elTextareas.length; textAreaIndex < numTextAreas; ++textAreaIndex) {
        var curTextAreaElement = elTextareas[textAreaIndex];
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
        var lis = dropdown.parentNode.getElementsByTagName("li");
        var text = "";
        for (var i = 0; i < lis.length; i++) {
            if (String(lis[i].getAttribute("value")) === String(dropdown.value)) {
                text = lis[i].textContent;
            }
        }
        dropdown.firstChild.textContent = text;
    }

    function toggleDropdown(event) {
        var element = event.target;
        if (element.nodeName !== "DT") {
            element = element.parentNode;
        }
        element = element.parentNode;
        var isDropped = element.getAttribute("dropped");
        element.setAttribute("dropped", isDropped !== "true" ? "true" : "false");
    }

    function setDropdownValue(event) {
        var dt = event.target.parentNode.parentNode.previousSibling;
        dt.value = event.target.getAttribute("value");
        dt.firstChild.textContent = event.target.textContent;

        dt.parentNode.setAttribute("dropped", "false");

        var evt = document.createEvent("HTMLEvents");
        evt.initEvent("change", true, true);
        dt.dispatchEvent(evt);
    }

    var elDropdowns = document.getElementsByTagName("select");
    for (var dropDownIndex = 0; dropDownIndex < elDropdowns.length; ++dropDownIndex) {
        var options = elDropdowns[dropDownIndex].getElementsByTagName("option");
        var selectedOption = 0;
        for (var optionIndex = 0; optionIndex < options.length; ++optionIndex) {
            if (options[optionIndex].getAttribute("selected") === "selected") {
                selectedOption = optionIndex;
                // TODO:  Shouldn't there be a break here?
            }
        }
        var div = elDropdowns[dropDownIndex].parentNode;

        var dl = document.createElement("dl");
        div.appendChild(dl);

        var dt = document.createElement("dt");
        dt.name = elDropdowns[dropDownIndex].name;
        dt.id = elDropdowns[dropDownIndex].id;
        dt.addEventListener("click", toggleDropdown, true);
        dl.appendChild(dt);

        var span = document.createElement("span");
        span.setAttribute("value", options[selectedOption].value);
        span.textContent = options[selectedOption].firstChild.textContent;
        dt.appendChild(span);

        var spanCaratDown = document.createElement("span");
        spanCaratDown.textContent = "5"; // caratDn
        dt.appendChild(spanCaratDown);

        var dd = document.createElement("dd");
        dl.appendChild(dd);

        var ul = document.createElement("ul");
        dd.appendChild(ul);

        for (var listOptionIndex = 0; listOptionIndex < options.length; ++listOptionIndex) {
            var li = document.createElement("li");
            li.setAttribute("value", options[listOptionIndex].value);
            li.textContent = options[listOptionIndex].firstChild.textContent;
            li.addEventListener("click", setDropdownValue);
            ul.appendChild(li);
        }
    }

    elDropdowns = document.getElementsByTagName("select");
    while (elDropdowns.length > 0) {
        var el = elDropdowns[0];
        el.parentNode.removeChild(el);
        elDropdowns = document.getElementsByTagName("select");
    }

    augmentSpinButtons();

    // Disable right-click context menu which is not visible in the HMD and makes it seem like the app has locked
    document.addEventListener("contextmenu", function(event) {
        event.preventDefault();
    }, false);

    setTimeout(function() {
        EventBridge.emitWebEvent(JSON.stringify({ type: 'propertiesPageReady' }));
    }, 1000);
}
