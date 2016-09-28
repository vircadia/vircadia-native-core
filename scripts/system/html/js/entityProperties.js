//  entityProperties.js
//
//  Created by Ryan Huffman on 13 Nov 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

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
    Text: "l",
    Light: "p",
    Zone: "o",
    PolyVox: "&#xe005;",
    Multiple: "&#xe000;"
}

var EDITOR_TIMEOUT_DURATION = 1500;

var colorPickers = [];
var lastEntityID = null;
debugPrint = function(message) {
    EventBridge.emitWebEvent(
        JSON.stringify({
            type: "print",
            message: message
        })
    );
};

function enableChildren(el, selector) {
    els = el.querySelectorAll(selector);
    for (var i = 0; i < els.length; i++) {
        els[i].removeAttribute('disabled');
    }
}

function disableChildren(el, selector) {
    els = el.querySelectorAll(selector);
    for (var i = 0; i < els.length; i++) {
        els[i].setAttribute('disabled', 'disabled');
    }
}

function enableProperties() {
    enableChildren(document.getElementById("properties-list"), "input, textarea, checkbox, .dropdown dl, .color-picker");
    enableChildren(document, ".colpick");
    var elLocked = document.getElementById("property-locked");

    if (elLocked.checked === false) {
        removeStaticUserData();
    }
}


function disableProperties() {
    disableChildren(document.getElementById("properties-list"), "input, textarea, checkbox, .dropdown dl, .color-picker");
    disableChildren(document, ".colpick");
    for (var i = 0; i < colorPickers.length; i++) {
        colorPickers[i].colpickHide();
    }
    var elLocked = document.getElementById("property-locked");

    if ($('#userdata-editor').css('display') === "block" && elLocked.checked === true) {
        showStaticUserData();
    }

}

function showElements(els, show) {
    for (var i = 0; i < els.length; i++) {
        els[i].style.display = (show) ? 'table' : 'none';
    }
}

function createEmitCheckedPropertyUpdateFunction(propertyName) {
    return function() {
        EventBridge.emitWebEvent(
            '{"id":' + lastEntityID + ', "type":"update", "properties":{"' + propertyName + '":' + this.checked + '}}'
        );
    };
}

function createEmitCheckedToStringPropertyUpdateFunction(checkboxElement, name, propertyName) {
    var newString = "";
    if (checkboxElement.checked) {
        newString += name + "";
    } else {

    }

}

function createEmitGroupCheckedPropertyUpdateFunction(group, propertyName) {
    return function() {
        var properties = {};
        properties[group] = {};
        properties[group][propertyName] = this.checked;
        EventBridge.emitWebEvent(
            JSON.stringify({
                id: lastEntityID,
                type: "update",
                properties: properties
            })
        );
    };
}

function createEmitNumberPropertyUpdateFunction(propertyName, decimals) {
    decimals = decimals == undefined ? 4 : decimals;
    return function() {
        var value = parseFloat(this.value).toFixed(decimals);

        EventBridge.emitWebEvent(
            '{"id":' + lastEntityID + ', "type":"update", "properties":{"' + propertyName + '":' + value + '}}'
        );
    };
}

function createEmitGroupNumberPropertyUpdateFunction(group, propertyName) {
    return function() {
        var properties = {};
        properties[group] = {};
        properties[group][propertyName] = this.value;
        EventBridge.emitWebEvent(
            JSON.stringify({
                id: lastEntityID,
                type: "update",
                properties: properties,
            })
        );
    };
}


function createEmitTextPropertyUpdateFunction(propertyName) {
    return function() {
        var properties = {};
        properties[propertyName] = this.value;
        EventBridge.emitWebEvent(
            JSON.stringify({
                id: lastEntityID,
                type: "update",
                properties: properties,
            })
        );
    };
}

function createEmitGroupTextPropertyUpdateFunction(group, propertyName) {
    return function() {
        var properties = {};
        properties[group] = {};
        properties[group][propertyName] = this.value;
        EventBridge.emitWebEvent(
            JSON.stringify({
                id: lastEntityID,
                type: "update",
                properties: properties,
            })
        );
    };
}

function createEmitVec3PropertyUpdateFunction(property, elX, elY, elZ) {
    return function() {
        var data = {
            id: lastEntityID,
            type: "update",
            properties: {}
        };
        data.properties[property] = {
            x: elX.value,
            y: elY.value,
            z: elZ.value,
        };
        EventBridge.emitWebEvent(JSON.stringify(data));
    }
};

function createEmitGroupVec3PropertyUpdateFunction(group, property, elX, elY, elZ) {
    return function() {
        var data = {
            id: lastEntityID,
            type: "update",
            properties: {}
        };
        data.properties[group] = {};
        data.properties[group][property] = {
            x: elX.value,
            y: elY.value,
            z: elZ ? elZ.value : 0,
        };
        EventBridge.emitWebEvent(JSON.stringify(data));
    }
};

function createEmitVec3PropertyUpdateFunctionWithMultiplier(property, elX, elY, elZ, multiplier) {
    return function() {
        var data = {
            id: lastEntityID,
            type: "update",
            properties: {}
        };
        data.properties[property] = {
            x: elX.value * multiplier,
            y: elY.value * multiplier,
            z: elZ.value * multiplier,
        };
        EventBridge.emitWebEvent(JSON.stringify(data));
    }
};

function createEmitColorPropertyUpdateFunction(property, elRed, elGreen, elBlue) {
    return function() {
        emitColorPropertyUpdate(property, elRed.value, elGreen.value, elBlue.value);
    }
};

function emitColorPropertyUpdate(property, red, green, blue, group) {
    var data = {
        id: lastEntityID,
        type: "update",
        properties: {}
    };
    if (group) {
        data.properties[group] = {};
        data.properties[group][property] = {
            red: red,
            green: green,
            blue: blue,
        };
    } else {
        data.properties[property] = {
            red: red,
            green: green,
            blue: blue,
        };
    }
    EventBridge.emitWebEvent(JSON.stringify(data));
};


function createEmitGroupColorPropertyUpdateFunction(group, property, elRed, elGreen, elBlue) {
    return function() {
        var data = {
            id: lastEntityID,
            type: "update",
            properties: {}
        };
        data.properties[group] = {};

        data.properties[group][property] = {
            red: elRed.value,
            green: elGreen.value,
            blue: elBlue.value,
        };
        EventBridge.emitWebEvent(JSON.stringify(data));
    }
};

function updateCheckedSubProperty(propertyName, propertyValue, subPropertyElement, subPropertyString) {
    if (subPropertyElement.checked) {
        if (propertyValue.indexOf(subPropertyString)) {
            propertyValue += subPropertyString + ',';
        }
    } else {
        // We've unchecked, so remove 
        propertyValue = propertyValue.replace(subPropertyString + ",", "");
    }

    var _properties = {}
    _properties[propertyName] = propertyValue;

    EventBridge.emitWebEvent(
        JSON.stringify({
            id: lastEntityID,
            type: "update",
            properties: _properties
        })
    );

}

function setUserDataFromEditor(noUpdate) {
    var json = null;
    try {
        json = editor.get();
    } catch (e) {
        alert('Invalid JSON code - look for red X in your code ', +e)
    }
    if (json === null) {
        return;
    } else {
        var text = editor.getText()
        if (noUpdate === true) {
            EventBridge.emitWebEvent(
                JSON.stringify({
                    id: lastEntityID,
                    type: "saveUserData",
                    properties: {
                        userData: text
                    },
                })
            );
            return;
        } else {
            EventBridge.emitWebEvent(
                JSON.stringify({
                    id: lastEntityID,
                    type: "update",
                    properties: {
                        userData: text
                    },
                })
            );
        }

    }


}

function userDataChanger(groupName, keyName, checkBoxElement, userDataElement, defaultValue) {
    var properties = {};
    var parsedData = {};
    try {
        if ($('#userdata-editor').css('height') !== "0px") {
            //if there is an expanded, we want to use its json.
            parsedData = getEditorJSON();
        } else {
            parsedData = JSON.parse(userDataElement.value);
        }

    } catch (e) {}

    if (!(groupName in parsedData)) {
        parsedData[groupName] = {}
    }
    delete parsedData[groupName][keyName];
    if (checkBoxElement.checked !== defaultValue) {
        parsedData[groupName][keyName] = checkBoxElement.checked;
    }

    if (Object.keys(parsedData[groupName]).length == 0) {
        delete parsedData[groupName];
    }
    if (Object.keys(parsedData).length > 0) {
        properties['userData'] = JSON.stringify(parsedData);
    } else {
        properties['userData'] = '';
    }

    userDataElement.value = properties['userData'];

    EventBridge.emitWebEvent(
        JSON.stringify({
            id: lastEntityID,
            type: "update",
            properties: properties,
        })
    );
};

function setTextareaScrolling(element) {
    var isScrolling = element.scrollHeight > element.offsetHeight;
    element.setAttribute("scrolling", isScrolling ? "true" : "false");
};



var editor = null;
var editorTimeout = null;
var lastJSONString = null;

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
            alert('JSON editor:' + e)
        },
        onChange: function() {
            var currentJSONString = editor.getText();

            if (currentJSONString === '{"":""}') {
                return;
            }
            $('#userdata-save').attr('disabled', false)


        }
    };
    editor = new JSONEditor(container, options);
};

function hideNewJSONEditorButton() {
    $('#userdata-new-editor').hide();

};

function hideClearUserDataButton() {
    $('#userdata-clear').hide();
};

function showSaveUserDataButton() {
    $('#userdata-save').show();
}

function hideSaveUserDataButton() {
    $('#userdata-save').hide();

}

function showNewJSONEditorButton() {
    $('#userdata-new-editor').show();

};

function showClearUserDataButton() {
    $('#userdata-clear').show();

};

function showUserDataTextArea() {
    $('#property-user-data').show();
};

function hideUserDataTextArea() {
    $('#property-user-data').hide();
};

function showStaticUserData() {
    $('#static-userdata').show();
    $('#static-userdata').css('height', $('#userdata-editor').height())
    if (editor !== null) {
        $('#static-userdata').text(editor.getText());
    }


};

function removeStaticUserData() {
    $('#static-userdata').hide();
};

function setEditorJSON(json) {
    editor.set(json)
    if (editor.hasOwnProperty('expandAll')) {
        editor.expandAll();
    }

};

function getEditorJSON() {
    return editor.get();
};

function deleteJSONEditor() {
    if (editor !== null) {
        editor.destroy();
        editor = null;
    }
};

var savedJSONTimer = null;

function saveJSONUserData(noUpdate) {
    setUserDataFromEditor(noUpdate);
    $('#userdata-saved').show();
    $('#userdata-save').attr('disabled', true)
    if (savedJSONTimer !== null) {
        clearTimeout(savedJSONTimer);
    }
    savedJSONTimer = setTimeout(function() {
        $('#userdata-saved').hide();

    }, 1500)
}

function bindAllNonJSONEditorElements() {
    var inputs = $('input');
    var i;
    for (i = 0; i < inputs.length; i++) {
        var input = inputs[i];
        var field = $(input);
        field.on('focus', function(e) {
            if (e.target.id === "userdata-new-editor" || e.target.id === "userdata-clear") {
                return;
            } else {
                if ($('#userdata-editor').css('height') !== "0px") {
                    saveJSONUserData(true);

                }
            }
        })
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

function loaded() {
    openEventBridge(function() {
        var allSections = [];
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
        var elWantsTrigger = document.getElementById("property-wants-trigger");
        var elIgnoreIK = document.getElementById("property-ignore-ik");

        var elLifetime = document.getElementById("property-lifetime");
        var elScriptURL = document.getElementById("property-script-url");
        /*
        FIXME: See FIXME for property-script-url.
        var elScriptTimestamp = document.getElementById("property-script-timestamp");
        */
        var elReloadScriptButton = document.getElementById("reload-script-button");
        var elUserData = document.getElementById("property-user-data");
        var elClearUserData = document.getElementById("userdata-clear");
        var elSaveUserData = document.getElementById("userdata-save");
        var elJSONEditor = document.getElementById("userdata-editor");
        var elNewJSONEditor = document.getElementById('userdata-new-editor');
        var elColorSections = document.querySelectorAll(".color-section");
        var elColor = document.getElementById("property-color");
        var elColorRed = document.getElementById("property-color-red");
        var elColorGreen = document.getElementById("property-color-green");
        var elColorBlue = document.getElementById("property-color-blue");

        var elShapeSections = document.querySelectorAll(".shape-section");
        allSections.push(elShapeSections);
        var elShape = document.getElementById("property-shape");

        var elLightSections = document.querySelectorAll(".light-section");
        allSections.push(elLightSections);
        var elLightSpotLight = document.getElementById("property-light-spot-light");
        var elLightColor = document.getElementById("property-light-color");
        var elLightColorRed = document.getElementById("property-light-color-red");
        var elLightColorGreen = document.getElementById("property-light-color-green");
        var elLightColorBlue = document.getElementById("property-light-color-blue");

        var elLightIntensity = document.getElementById("property-light-intensity");
        var elLightFalloffRadius = document.getElementById("property-light-falloff-radius");
        var elLightExponent = document.getElementById("property-light-exponent");
        var elLightCutoff = document.getElementById("property-light-cutoff");

        var elModelSections = document.querySelectorAll(".model-section");
        allSections.push(elModelSections);
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
        var elModelTextures = document.getElementById("property-model-textures");
        var elModelOriginalTextures = document.getElementById("property-model-original-textures");

        var elWebSections = document.querySelectorAll(".web-section");
        allSections.push(elWebSections);
        var elWebSourceURL = document.getElementById("property-web-source-url");
        var elWebDPI = document.getElementById("property-web-dpi");

        var elDescription = document.getElementById("property-description");

        var elHyperlinkHref = document.getElementById("property-hyperlink-href");

        var elHyperlinkSections = document.querySelectorAll(".hyperlink-section");


        var elTextSections = document.querySelectorAll(".text-section");
        allSections.push(elTextSections);
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

        var elZoneSections = document.querySelectorAll(".zone-section");
        allSections.push(elZoneSections);
        var elZoneStageSunModelEnabled = document.getElementById("property-zone-stage-sun-model-enabled");

        var elZoneKeyLightColor = document.getElementById("property-zone-key-light-color");
        var elZoneKeyLightColorRed = document.getElementById("property-zone-key-light-color-red");
        var elZoneKeyLightColorGreen = document.getElementById("property-zone-key-light-color-green");
        var elZoneKeyLightColorBlue = document.getElementById("property-zone-key-light-color-blue");
        var elZoneKeyLightIntensity = document.getElementById("property-zone-key-intensity");
        var elZoneKeyLightAmbientIntensity = document.getElementById("property-zone-key-ambient-intensity");
        var elZoneKeyLightDirectionX = document.getElementById("property-zone-key-light-direction-x");
        var elZoneKeyLightDirectionY = document.getElementById("property-zone-key-light-direction-y");
        var elZoneKeyLightDirectionZ = document.getElementById("property-zone-key-light-direction-z");
        var elZoneKeyLightAmbientURL = document.getElementById("property-zone-key-ambient-url");

        var elZoneStageLatitude = document.getElementById("property-zone-stage-latitude");
        var elZoneStageLongitude = document.getElementById("property-zone-stage-longitude");
        var elZoneStageAltitude = document.getElementById("property-zone-stage-altitude");
        var elZoneStageAutomaticHourDay = document.getElementById("property-zone-stage-automatic-hour-day");
        var elZoneStageDay = document.getElementById("property-zone-stage-day");
        var elZoneStageHour = document.getElementById("property-zone-stage-hour");

        var elZoneBackgroundMode = document.getElementById("property-zone-background-mode");

        var elZoneSkyboxColor = document.getElementById("property-zone-skybox-color");
        var elZoneSkyboxColorRed = document.getElementById("property-zone-skybox-color-red");
        var elZoneSkyboxColorGreen = document.getElementById("property-zone-skybox-color-green");
        var elZoneSkyboxColorBlue = document.getElementById("property-zone-skybox-color-blue");
        var elZoneSkyboxURL = document.getElementById("property-zone-skybox-url");

        var elZoneFlyingAllowed = document.getElementById("property-zone-flying-allowed");
        var elZoneGhostingAllowed = document.getElementById("property-zone-ghosting-allowed");

        var elPolyVoxSections = document.querySelectorAll(".poly-vox-section");
        allSections.push(elPolyVoxSections);
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
                if (data.type == "update") {

                    if (data.selections.length == 0) {
                        if (editor !== null && lastEntityID !== null) {
                            saveJSONUserData(true);
                            deleteJSONEditor();
                        }
                        elTypeIcon.style.display = "none";
                        elType.innerHTML = "<i>No selection</i>";
                        elID.innerHTML = "";
                        disableProperties();
                    } else if (data.selections.length > 1) {
                        deleteJSONEditor();
                        var selections = data.selections;

                        var ids = [];
                        var types = {};
                        var numTypes = 0;

                        for (var i = 0; i < selections.length; i++) {
                            ids.push(selections[i].id);
                            var type = selections[i].properties.type;
                            if (types[type] === undefined) {
                                types[type] = 0;
                                numTypes += 1;
                            }
                            types[type]++;
                        }

                        var type;
                        if (numTypes === 1) {
                            type = selections[0].properties.type;
                        } else {
                            type = "Multiple";
                        }
                        elType.innerHTML = type + " (" + data.selections.length + ")";
                        elTypeIcon.innerHTML = ICON_FOR_TYPE[type];
                        elTypeIcon.style.display = "inline-block";

                        elID.innerHTML = ids.join("<br>");

                        disableProperties();
                    } else {

                        properties = data.selections[0].properties;

                        if (lastEntityID !== '"' + properties.id + '"' && lastEntityID !== null && editor !== null) {
                            saveJSONUserData(true);
                        }
                        //the event bridge and json parsing handle our avatar id string differently.  

                        lastEntityID = '"' + properties.id + '"';
                        elID.innerHTML = properties.id;

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
                        elWantsTrigger.checked = false;
                        elIgnoreIK.checked = false;
                        var parsedUserData = {}
                        try {
                            parsedUserData = JSON.parse(properties.userData);

                            if ("grabbableKey" in parsedUserData) {
                                if ("grabbable" in parsedUserData["grabbableKey"]) {
                                    elGrabbable.checked = parsedUserData["grabbableKey"].grabbable;
                                }
                                if ("wantsTrigger" in parsedUserData["grabbableKey"]) {
                                    elWantsTrigger.checked = parsedUserData["grabbableKey"].wantsTrigger;
                                }
                                if ("ignoreIK" in parsedUserData["grabbableKey"]) {
                                    elIgnoreIK.checked = parsedUserData["grabbableKey"].ignoreIK;
                                }
                            }
                        } catch (e) {}

                        elCollisionSoundURL.value = properties.collisionSoundURL;
                        elLifetime.value = properties.lifetime;
                        elScriptURL.value = properties.script;
                        /*
                        FIXME: See FIXME for property-script-url.
                        elScriptTimestamp.value = properties.scriptTimestamp;
                        */

                        var json = null;
                        try {
                            json = JSON.parse(properties.userData);
                        } catch (e) {
                            //normal text
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

                        elHyperlinkHref.value = properties.href;
                        elDescription.value = properties.description;

                        for (var i = 0; i < allSections.length; i++) {
                            for (var j = 0; j < allSections[i].length; j++) {
                                allSections[i][j].style.display = 'none';
                            }
                        }

                        for (var i = 0; i < elHyperlinkSections.length; i++) {
                            elHyperlinkSections[i].style.display = 'table';
                        }

                        if (properties.type == "Shape" || properties.type == "Box" || properties.type == "Sphere") {
                            for (var i = 0; i < elShapeSections.length; i++) {
                                elShapeSections[i].style.display = 'table';
                            }
                            elShape.value = properties.shape;
                            setDropdownText(elShape);

                        } else {
                            for (var i = 0; i < elShapeSections.length; i++) {
                                elShapeSections[i].style.display = 'none';
                            }
                        }

                        if (properties.type == "Shape" || properties.type == "Box" || properties.type == "Sphere" || properties.type == "ParticleEffect") {
                            for (var i = 0; i < elColorSections.length; i++) {
                                elColorSections[i].style.display = 'table';
                            }
                            elColorRed.value = properties.color.red;
                            elColorGreen.value = properties.color.green;
                            elColorBlue.value = properties.color.blue;
                            elColor.style.backgroundColor = "rgb(" + properties.color.red + "," + properties.color.green + "," + properties.color.blue + ")";
                        } else {
                            for (var i = 0; i < elColorSections.length; i++) {
                                elColorSections[i].style.display = 'none';
                            }
                        }

                        if (properties.type == "Model") {
                            for (var i = 0; i < elModelSections.length; i++) {
                                elModelSections[i].style.display = 'table';
                            }

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
                            elModelTextures.value = properties.textures;
                            setTextareaScrolling(elModelTextures);
                            elModelOriginalTextures.value = properties.originalTextures;
                            setTextareaScrolling(elModelOriginalTextures);
                        } else if (properties.type == "Web") {
                            for (var i = 0; i < elWebSections.length; i++) {
                                elWebSections[i].style.display = 'table';
                            }
                            for (var i = 0; i < elHyperlinkSections.length; i++) {
                                elHyperlinkSections[i].style.display = 'none';
                            }

                            elWebSourceURL.value = properties.sourceUrl;
                            elWebDPI.value = properties.dpi;
                        } else if (properties.type == "Text") {
                            for (var i = 0; i < elTextSections.length; i++) {
                                elTextSections[i].style.display = 'table';
                            }

                            elTextText.value = properties.text;
                            elTextLineHeight.value = properties.lineHeight.toFixed(4);
                            elTextFaceCamera = properties.faceCamera;
                            elTextTextColor.style.backgroundColor = "rgb(" + properties.textColor.red + "," + properties.textColor.green + "," + properties.textColor.blue + ")";
                            elTextTextColorRed.value = properties.textColor.red;
                            elTextTextColorGreen.value = properties.textColor.green;
                            elTextTextColorBlue.value = properties.textColor.blue;
                            elTextBackgroundColorRed.value = properties.backgroundColor.red;
                            elTextBackgroundColorGreen.value = properties.backgroundColor.green;
                            elTextBackgroundColorBlue.value = properties.backgroundColor.blue;
                        } else if (properties.type == "Light") {
                            for (var i = 0; i < elLightSections.length; i++) {
                                elLightSections[i].style.display = 'table';
                            }

                            elLightSpotLight.checked = properties.isSpotlight;

                            elLightColor.style.backgroundColor = "rgb(" + properties.color.red + "," + properties.color.green + "," + properties.color.blue + ")";
                            elLightColorRed.value = properties.color.red;
                            elLightColorGreen.value = properties.color.green;
                            elLightColorBlue.value = properties.color.blue;

                            elLightIntensity.value = properties.intensity.toFixed(1);
                            elLightFalloffRadius.value = properties.falloffRadius.toFixed(1);
                            elLightExponent.value = properties.exponent.toFixed(2);
                            elLightCutoff.value = properties.cutoff.toFixed(2);
                        } else if (properties.type == "Zone") {
                            for (var i = 0; i < elZoneSections.length; i++) {
                                elZoneSections[i].style.display = 'table';
                            }

                            elZoneStageSunModelEnabled.checked = properties.stage.sunModelEnabled;
                            elZoneKeyLightColor.style.backgroundColor = "rgb(" + properties.keyLight.color.red + "," + properties.keyLight.color.green + "," + properties.keyLight.color.blue + ")";
                            elZoneKeyLightColorRed.value = properties.keyLight.color.red;
                            elZoneKeyLightColorGreen.value = properties.keyLight.color.green;
                            elZoneKeyLightColorBlue.value = properties.keyLight.color.blue;
                            elZoneKeyLightIntensity.value = properties.keyLight.intensity.toFixed(2);
                            elZoneKeyLightAmbientIntensity.value = properties.keyLight.ambientIntensity.toFixed(2);
                            elZoneKeyLightDirectionX.value = properties.keyLight.direction.x.toFixed(2);
                            elZoneKeyLightDirectionY.value = properties.keyLight.direction.y.toFixed(2);
                            elZoneKeyLightAmbientURL.value = properties.keyLight.ambientURL;


                            elZoneStageLatitude.value = properties.stage.latitude.toFixed(2);
                            elZoneStageLongitude.value = properties.stage.longitude.toFixed(2);
                            elZoneStageAltitude.value = properties.stage.altitude.toFixed(2);
                            elZoneStageAutomaticHourDay.checked = properties.stage.automaticHourDay;
                            elZoneStageDay.value = properties.stage.day;
                            elZoneStageHour.value = properties.stage.hour;
                            elShapeType.value = properties.shapeType;
                            elCompoundShapeURL.value = properties.compoundShapeURL;

                            elZoneBackgroundMode.value = properties.backgroundMode;
                            setDropdownText(elZoneBackgroundMode);

                            elZoneSkyboxColor.style.backgroundColor = "rgb(" + properties.skybox.color.red + "," + properties.skybox.color.green + "," + properties.skybox.color.blue + ")";
                            elZoneSkyboxColorRed.value = properties.skybox.color.red;
                            elZoneSkyboxColorGreen.value = properties.skybox.color.green;
                            elZoneSkyboxColorBlue.value = properties.skybox.color.blue;
                            elZoneSkyboxURL.value = properties.skybox.url;

                            elZoneFlyingAllowed.checked = properties.flyingAllowed;
                            elZoneGhostingAllowed.checked = properties.ghostingAllowed;

                            showElements(document.getElementsByClassName('skybox-section'), elZoneBackgroundMode.value == 'skybox');
                        } else if (properties.type == "PolyVox") {
                            for (var i = 0; i < elPolyVoxSections.length; i++) {
                                elPolyVoxSections[i].style.display = 'table';
                            }

                            elVoxelVolumeSizeX.value = properties.voxelVolumeSize.x.toFixed(2);
                            elVoxelVolumeSizeY.value = properties.voxelVolumeSize.y.toFixed(2);
                            elVoxelVolumeSizeZ.value = properties.voxelVolumeSize.z.toFixed(2);
                            elVoxelSurfaceStyle.value = properties.voxelSurfaceStyle;
                            setDropdownText(elVoxelSurfaceStyle);
                            elXTextureURL.value = properties.xTextureURL;
                            elYTextureURL.value = properties.yTextureURL;
                            elZTextureURL.value = properties.zTextureURL;
                        }

                        if (properties.locked) {
                            disableProperties();
                            elLocked.removeAttribute('disabled');
                        } else {
                            enableProperties();
                            elSaveUserData.disabled = true;
                        }

                        var activeElement = document.activeElement;

                        if (typeof activeElement.select !== "undefined") {
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
        elParentJointIndex.addEventListener('change', createEmitNumberPropertyUpdateFunction('parentJointIndex'));

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
            userDataChanger("grabbableKey", "grabbable", elGrabbable, elUserData, properties.dynamic);
        });
        elWantsTrigger.addEventListener('change', function() {
            userDataChanger("grabbableKey", "wantsTrigger", elWantsTrigger, elUserData, false);
        });
        elIgnoreIK.addEventListener('change', function() {
            userDataChanger("grabbableKey", "ignoreIK", elIgnoreIK, elUserData, false);
        });

        elCollisionSoundURL.addEventListener('change', createEmitTextPropertyUpdateFunction('collisionSoundURL'));

        elLifetime.addEventListener('change', createEmitNumberPropertyUpdateFunction('lifetime'));
        elScriptURL.addEventListener('change', createEmitTextPropertyUpdateFunction('script'));
        /*
        FIXME: See FIXME for property-script-url.
        elScriptTimestamp.addEventListener('change', createEmitNumberPropertyUpdateFunction('scriptTimestamp'));
        */


        elClearUserData.addEventListener("click", function() {
            deleteJSONEditor();
            elUserData.value = "";
            showUserDataTextArea();
            showNewJSONEditorButton();
            hideSaveUserDataButton();
            var properties = {};
            properties['userData'] = elUserData.value;
            EventBridge.emitWebEvent(
                JSON.stringify({
                    id: lastEntityID,
                    type: "update",
                    properties: properties,
                })
            );


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

        var colorChangeFunction = createEmitColorPropertyUpdateFunction(
            'color', elColorRed, elColorGreen, elColorBlue);
        elColorRed.addEventListener('change', colorChangeFunction);
        elColorGreen.addEventListener('change', colorChangeFunction);
        elColorBlue.addEventListener('change', colorChangeFunction);
        colorPickers.push($('#property-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            onShow: function(colpick) {
                $('#property-color').attr('active', 'true');
            },
            onHide: function(colpick) {
                $('#property-color').attr('active', 'false');
            },
            onSubmit: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                $(el).colpickHide();
                emitColorPropertyUpdate('color', rgb.r, rgb.g, rgb.b);
            }
        }));

        elLightSpotLight.addEventListener('change', createEmitCheckedPropertyUpdateFunction('isSpotlight'));

        var lightColorChangeFunction = createEmitColorPropertyUpdateFunction(
            'color', elLightColorRed, elLightColorGreen, elLightColorBlue);
        elLightColorRed.addEventListener('change', lightColorChangeFunction);
        elLightColorGreen.addEventListener('change', lightColorChangeFunction);
        elLightColorBlue.addEventListener('change', lightColorChangeFunction);
        colorPickers.push($('#property-light-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            onShow: function(colpick) {
                $('#property-light-color').attr('active', 'true');
            },
            onHide: function(colpick) {
                $('#property-light-color').attr('active', 'false');
            },
            onSubmit: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                $(el).colpickHide();
                emitColorPropertyUpdate('color', rgb.r, rgb.g, rgb.b);
            }
        }));

        elLightIntensity.addEventListener('change', createEmitNumberPropertyUpdateFunction('intensity', 1));
        elLightFalloffRadius.addEventListener('change', createEmitNumberPropertyUpdateFunction('falloffRadius', 1));
        elLightExponent.addEventListener('change', createEmitNumberPropertyUpdateFunction('exponent', 2));
        elLightCutoff.addEventListener('change', createEmitNumberPropertyUpdateFunction('cutoff', 2));

        elShape.addEventListener('change', createEmitTextPropertyUpdateFunction('shape'));

        elWebSourceURL.addEventListener('change', createEmitTextPropertyUpdateFunction('sourceUrl'));
        elWebDPI.addEventListener('change', createEmitNumberPropertyUpdateFunction('dpi'));

        elModelURL.addEventListener('change', createEmitTextPropertyUpdateFunction('modelURL'));
        elShapeType.addEventListener('change', createEmitTextPropertyUpdateFunction('shapeType'));
        elCompoundShapeURL.addEventListener('change', createEmitTextPropertyUpdateFunction('compoundShapeURL'));

        elModelAnimationURL.addEventListener('change', createEmitGroupTextPropertyUpdateFunction('animation', 'url'));
        elModelAnimationPlaying.addEventListener('change', createEmitGroupCheckedPropertyUpdateFunction('animation', 'running'));
        elModelAnimationFPS.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('animation', 'fps'));
        elModelAnimationFrame.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('animation', 'currentFrame'));
        elModelAnimationFirstFrame.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('animation', 'firstFrame'));
        elModelAnimationLastFrame.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('animation', 'lastFrame'));
        elModelAnimationLoop.addEventListener('change', createEmitGroupCheckedPropertyUpdateFunction('animation', 'loop'));
        elModelAnimationHold.addEventListener('change', createEmitGroupCheckedPropertyUpdateFunction('animation', 'hold'));

        elModelTextures.addEventListener('change', createEmitTextPropertyUpdateFunction('textures'));

        elTextText.addEventListener('change', createEmitTextPropertyUpdateFunction('text'));
        elTextFaceCamera.addEventListener('change', createEmitCheckedPropertyUpdateFunction('faceCamera'));
        elTextLineHeight.addEventListener('change', createEmitNumberPropertyUpdateFunction('lineHeight'));
        var textTextColorChangeFunction = createEmitColorPropertyUpdateFunction(
            'textColor', elTextTextColorRed, elTextTextColorGreen, elTextTextColorBlue);
        elTextTextColorRed.addEventListener('change', textTextColorChangeFunction);
        elTextTextColorGreen.addEventListener('change', textTextColorChangeFunction);
        elTextTextColorBlue.addEventListener('change', textTextColorChangeFunction);
        colorPickers.push($('#property-text-text-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            onShow: function(colpick) {
                $('#property-text-text-color').attr('active', 'true');
            },
            onHide: function(colpick) {
                $('#property-text-text-color').attr('active', 'false');
            },
            onSubmit: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                $(el).colpickHide();
                $(el).attr('active', 'false');
                emitColorPropertyUpdate('textColor', rgb.r, rgb.g, rgb.b);
            }
        }));

        var textBackgroundColorChangeFunction = createEmitColorPropertyUpdateFunction(
            'backgroundColor', elTextBackgroundColorRed, elTextBackgroundColorGreen, elTextBackgroundColorBlue);
        elTextBackgroundColorRed.addEventListener('change', textBackgroundColorChangeFunction);
        elTextBackgroundColorGreen.addEventListener('change', textBackgroundColorChangeFunction);
        elTextBackgroundColorBlue.addEventListener('change', textBackgroundColorChangeFunction);
        colorPickers.push($('#property-text-background-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            onShow: function(colpick) {
                $('#property-text-background-color').attr('active', 'true');
            },
            onHide: function(colpick) {
                $('#property-text-background-color').attr('active', 'false');
            },
            onSubmit: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                $(el).colpickHide();
                emitColorPropertyUpdate('backgroundColor', rgb.r, rgb.g, rgb.b);
            }
        }));

        elZoneStageSunModelEnabled.addEventListener('change', createEmitGroupCheckedPropertyUpdateFunction('stage', 'sunModelEnabled'));
        colorPickers.push($('#property-zone-key-light-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            onShow: function(colpick) {
                $('#property-zone-key-light-color').attr('active', 'true');
            },
            onHide: function(colpick) {
                $('#property-zone-key-light-color').attr('active', 'false');
            },
            onSubmit: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                $(el).colpickHide();
                emitColorPropertyUpdate('color', rgb.r, rgb.g, rgb.b, 'keyLight');
            }
        }));
        var zoneKeyLightColorChangeFunction = createEmitGroupColorPropertyUpdateFunction('keyLight', 'color', elZoneKeyLightColorRed, elZoneKeyLightColorGreen, elZoneKeyLightColorBlue);
        elZoneKeyLightColorRed.addEventListener('change', zoneKeyLightColorChangeFunction);
        elZoneKeyLightColorGreen.addEventListener('change', zoneKeyLightColorChangeFunction);
        elZoneKeyLightColorBlue.addEventListener('change', zoneKeyLightColorChangeFunction);
        elZoneKeyLightIntensity.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('keyLight', 'intensity'));
        elZoneKeyLightAmbientIntensity.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('keyLight', 'ambientIntensity'));
        elZoneKeyLightAmbientURL.addEventListener('change', createEmitGroupTextPropertyUpdateFunction('keyLight', 'ambientURL'));
        var zoneKeyLightDirectionChangeFunction = createEmitGroupVec3PropertyUpdateFunction('keyLight', 'direction', elZoneKeyLightDirectionX, elZoneKeyLightDirectionY);
        elZoneKeyLightDirectionX.addEventListener('change', zoneKeyLightDirectionChangeFunction);
        elZoneKeyLightDirectionY.addEventListener('change', zoneKeyLightDirectionChangeFunction);

        elZoneStageLatitude.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('stage', 'latitude'));
        elZoneStageLongitude.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('stage', 'longitude'));
        elZoneStageAltitude.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('stage', 'altitude'));
        elZoneStageAutomaticHourDay.addEventListener('change', createEmitGroupCheckedPropertyUpdateFunction('stage', 'automaticHourDay'));
        elZoneStageDay.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('stage', 'day'));
        elZoneStageHour.addEventListener('change', createEmitGroupNumberPropertyUpdateFunction('stage', 'hour'));


        elZoneBackgroundMode.addEventListener('change', createEmitTextPropertyUpdateFunction('backgroundMode'));
        var zoneSkyboxColorChangeFunction = createEmitGroupColorPropertyUpdateFunction('skybox', 'color',
            elZoneSkyboxColorRed, elZoneSkyboxColorGreen, elZoneSkyboxColorBlue);
        elZoneSkyboxColorRed.addEventListener('change', zoneSkyboxColorChangeFunction);
        elZoneSkyboxColorGreen.addEventListener('change', zoneSkyboxColorChangeFunction);
        elZoneSkyboxColorBlue.addEventListener('change', zoneSkyboxColorChangeFunction);
        colorPickers.push($('#property-zone-skybox-color').colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: '000000',
            onShow: function(colpick) {
                $('#property-zone-skybox-color').attr('active', 'true');
            },
            onHide: function(colpick) {
                $('#property-zone-skybox-color').attr('active', 'false');
            },
            onSubmit: function(hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                $(el).colpickHide();
                emitColorPropertyUpdate('color', rgb.r, rgb.g, rgb.b, 'skybox');
            }
        }));

        elZoneSkyboxURL.addEventListener('change', createEmitGroupTextPropertyUpdateFunction('skybox', 'url'));

        elZoneFlyingAllowed.addEventListener('change', createEmitCheckedPropertyUpdateFunction('flyingAllowed'));
        elZoneGhostingAllowed.addEventListener('change', createEmitCheckedPropertyUpdateFunction('ghostingAllowed'));

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
                action: "moveSelectionToGrid",
            }));
        });
        elMoveAllToGrid.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "moveAllToGrid",
            }));
        });
        elResetToNaturalDimensions.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "resetToNaturalDimensions",
            }));
        });
        elRescaleDimensionsButton.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "rescaleDimensions",
                percentage: parseInt(elRescaleDimensionsPct.value),
            }));
        });
        /*
        FIXME: See FIXME for property-script-url.
        elReloadScriptButton.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "reloadScript"
            }));
        });
        */

        window.onblur = function() {
            // Fake a change event
            var ev = document.createEvent("HTMLEvents");
            ev.initEvent("change", true, true);
            document.activeElement.dispatchEvent(ev);
        }

        // For input and textarea elements, select all of the text on focus
        // WebKit-based browsers, such as is used with QWebView, have a quirk
        // where the mouseup event comes after the focus event, causing the
        // text to be deselected immediately after selecting all of the text.
        // To make this work we block the first mouseup event after the elements
        // received focus.  If we block all mouseup events the user will not
        // be able to click within the selected text.
        // We also check to see if the value has changed to make sure we aren't
        // blocking a mouse-up event when clicking on an input spinner.
        var els = document.querySelectorAll("input, textarea");
        for (var i = 0; i < els.length; i++) {
            var clicked = false;
            var originalText;
            els[i].onfocus = function(e) {
                originalText = this.value;
                this.select();
                clicked = false;
            };
            els[i].onmouseup = function(e) {
                if (!clicked && originalText == this.value) {
                    e.preventDefault();
                }
                clicked = true;
            };
        }
        bindAllNonJSONEditorElements();
    });

    // Collapsible sections
    var elCollapsible = document.getElementsByClassName("section-header");

    var toggleCollapsedEvent = function(event) {
        var element = event.target;
        if (element.nodeName !== "DIV") {
            element = element.parentNode;
        }
        var isCollapsed = element.getAttribute("collapsed") !== "true";
        element.setAttribute("collapsed", isCollapsed ? "true" : "false");
        element.getElementsByTagName("span")[0].textContent = isCollapsed ? "L" : "M";
    };

    for (var i = 0, length = elCollapsible.length; i < length; i++) {
        var element = elCollapsible[i];
        element.addEventListener("click", toggleCollapsedEvent, true);
    };


    // Textarea scrollbars
    var elTextareas = document.getElementsByTagName("TEXTAREA");

    var textareaOnChangeEvent = function(event) {
        setTextareaScrolling(event.target);
    }

    for (var i = 0, length = elTextareas.length; i < length; i++) {
        var element = elTextareas[i];
        setTextareaScrolling(element);
        element.addEventListener("input", textareaOnChangeEvent, false);
        element.addEventListener("change", textareaOnChangeEvent, false);
        /* FIXME: Detect and update textarea scrolling attribute on resize. Unfortunately textarea doesn't have a resize
        event; mouseup is a partial stand-in but doesn't handle resizing if mouse moves outside textarea rectangle. */
        element.addEventListener("mouseup", textareaOnChangeEvent, false);
    };

    // Dropdowns
    // For each dropdown the following replacement is created in place of the oriringal dropdown...
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
            if (lis[i].getAttribute("value") === dropdown.value) {
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
    for (var i = 0; i < elDropdowns.length; i++) {
        var options = elDropdowns[i].getElementsByTagName("option");
        var selectedOption = 0;
        for (var j = 0; j < options.length; j++) {
            if (options[j].getAttribute("selected") === "selected") {
                selectedOption = j;
            }
        }
        var div = elDropdowns[i].parentNode;

        var dl = document.createElement("dl");
        div.appendChild(dl);

        var dt = document.createElement("dt");
        dt.name = elDropdowns[i].name;
        dt.id = elDropdowns[i].id;
        dt.addEventListener("click", toggleDropdown, true);
        dl.appendChild(dt);

        var span = document.createElement("span");
        span.setAttribute("value", options[selectedOption].value);
        span.textContent = options[selectedOption].firstChild.textContent;
        dt.appendChild(span);

        var span = document.createElement("span");
        span.textContent = "5"; // caratDn
        dt.appendChild(span);

        var dd = document.createElement("dd");
        dl.appendChild(dd);

        var ul = document.createElement("ul");
        dd.appendChild(ul);

        for (var j = 0; j < options.length; j++) {
            var li = document.createElement("li");
            li.setAttribute("value", options[j].value);
            li.textContent = options[j].firstChild.textContent;
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

    setUpKeyboardControl();

    // Disable right-click context menu which is not visible in the HMD and makes it seem like the app has locked
    document.addEventListener("contextmenu", function(event) {
        event.preventDefault();
    }, false);
}