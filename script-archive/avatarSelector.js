//
//  avatarSelector.js
//  examples
//
//  Created by David Rowe on 21 Apr 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Based on lobby.js created by Stephen Birarda on 17 Oct 2014.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var VIRCADIA_PUBLIC_CDN = networkingConstants.PUBLIC_BUCKET_CDN_URL;

var panelWall = false;
var orbShell = false;
var descriptionText = false;
var showText = false;

// used for formating the description text, in meters
var textWidth = 4;
var textHeight = .5;
var numberOfLines = 2;
var textMargin = 0.0625;
var lineHeight = (textHeight - (2 * textMargin)) / numberOfLines;

var avatarStickPosition = {};

var orbNaturalExtentsMin = { x: -1.230354, y: -1.22077, z: -1.210487 };
var orbNaturalExtentsMax = { x: 1.230353, y: 1.229819, z: 1.210487 };
var panelsNaturalExtentsMin = { x: -1.223182, y: -0.348487, z: 0.0451369 };
var panelsNaturalExtentsMax = { x: 1.223039, y: 0.602978, z: 1.224298 };

var orbNaturalDimensions = Vec3.subtract(orbNaturalExtentsMax, orbNaturalExtentsMin);
var panelsNaturalDimensions = Vec3.subtract(panelsNaturalExtentsMax, panelsNaturalExtentsMin);

var SCALING_FACTOR = 10;
var orbDimensions = Vec3.multiply(orbNaturalDimensions, SCALING_FACTOR);
var panelsDimensions = Vec3.multiply(panelsNaturalDimensions, SCALING_FACTOR);

var orbNaturalCenter = Vec3.sum(orbNaturalExtentsMin, Vec3.multiply(orbNaturalDimensions, 0.5));
var panelsNaturalCenter = Vec3.sum(panelsNaturalExtentsMin, Vec3.multiply(panelsNaturalDimensions, 0.5));
var orbCenter = Vec3.multiply(orbNaturalCenter, SCALING_FACTOR);
var panelsCenter = Vec3.multiply(panelsNaturalCenter, SCALING_FACTOR);
var panelsCenterShift = Vec3.subtract(panelsCenter, orbCenter);

var ORB_SHIFT = { x: 0, y: -1.4, z: -0.8 };

var LOBBY_PANEL_WALL_URL = VIRCADIA_PUBLIC_CDN + "models/sets/Lobby/PanelWallForInterface.fbx";
var LOBBY_BLANK_PANEL_TEXTURE_URL = VIRCADIA_PUBLIC_CDN + "models/sets/Lobby/Texture.jpg";
var LOBBY_SHELL_URL = VIRCADIA_PUBLIC_CDN + "models/sets/Lobby/LobbyShellForInterface.fbx";

var droneSound = SoundCache.getSound(VIRCADIA_PUBLIC_CDN + "sounds/Lobby/drone.stereo.raw")
var currentDrone = null;

var latinSound = SoundCache.getSound(VIRCADIA_PUBLIC_CDN + "sounds/Lobby/latin.stereo.raw")
var latinInjector = null;
var elevatorSound = SoundCache.getSound(VIRCADIA_PUBLIC_CDN + "sounds/Lobby/elevator.stereo.raw")
var elevatorInjector = null;
var currentMuzakInjector = null;
var currentSound = null;

function textOverlayPosition() {
    var TEXT_DISTANCE_OUT = 6;
    var TEXT_DISTANCE_DOWN = -2;
    return Vec3.sum(Vec3.sum(Camera.position, Vec3.multiply(Quat.getFront(Camera.orientation), TEXT_DISTANCE_OUT)),
                                Vec3.multiply(Quat.getUp(Camera.orientation), TEXT_DISTANCE_DOWN));
}

var panelPlaceOrder = [
  7, 8, 9, 10, 11, 12, 13,
  0, 1, 2, 3, 4, 5, 6,
  14, 15, 16, 17, 18, 19, 20
];

// Avatar index is 0-based
function avatarIndexToPanelIndex(avatarIndex) {
    return panelPlaceOrder.indexOf(avatarIndex) + 1;
}

// Panel index is 1-based
function panelIndexToPlaceIndex(panelIndex) {
    return panelPlaceOrder[panelIndex - 1];
}

var MAX_NUM_PANELS = 21;
var DRONE_VOLUME = 0.3;

function drawLobby() {
    if (!panelWall) {
        print("Adding overlays for the avatar selector panel wall and orb shell.");

        var cameraEuler = Quat.safeEulerAngles(Camera.orientation);
        var towardsMe = Quat.angleAxis(cameraEuler.y + 180, { x: 0, y: 1, z: 0 });

        var orbPosition = Vec3.sum(Camera.position, Vec3.multiplyQbyV(towardsMe, ORB_SHIFT));

        var panelWallProps = {
            url: LOBBY_PANEL_WALL_URL,
            position: Vec3.sum(orbPosition, Vec3.multiplyQbyV(towardsMe, panelsCenterShift)),
            rotation: towardsMe,
            dimensions: panelsDimensions
        };

        var orbShellProps = {
            url: LOBBY_SHELL_URL,
            position: orbPosition,
            rotation: towardsMe,
            dimensions: orbDimensions,
            ignoreRayIntersection: true
        };

        var windowDimensions = Controller.getViewportDimensions();

        var descriptionTextProps = {
            position: textOverlayPosition(),
            dimensions: { x: textWidth, y: textHeight },
            backgroundColor: { red: 0, green: 0, blue: 0 },
            color: { red: 255, green: 255, blue: 255 },
            topMargin: textMargin,
            leftMargin: textMargin,
            bottomMargin: textMargin,
            rightMargin: textMargin,
            text: "",
            lineHeight: lineHeight,
            alpha: 0.9,
            backgroundAlpha: 0.9,
            ignoreRayIntersection: true,
            visible: false,
            isFacingAvatar: true
        };

        avatarStickPosition = MyAvatar.position;

        panelWall = Overlays.addOverlay("model", panelWallProps);
        orbShell = Overlays.addOverlay("model", orbShellProps);
        descriptionText = Overlays.addOverlay("text3d", descriptionTextProps);

        if (droneSound.downloaded) {
            // start the drone sound
            if (!currentDrone) {
                currentDrone = Audio.playSound(droneSound, { stereo: true, loop: true, localOnly: true, volume: DRONE_VOLUME });
            } else {
                currentDrone.restart();
            }
        }

        // start one of our muzak sounds
        playRandomMuzak();
    }
}

var avatars = {};

function changeLobbyTextures() {
    var req = new XMLHttpRequest();
    req.open("GET", "https://metaverse.highfidelity.com/api/v1/marketplace?category=head+%26+body&limit=21", false);
    req.send();  // Data returned is randomized.

    avatars = JSON.parse(req.responseText).data.items;

    var NUM_PANELS = avatars.length;

    var textureProp = {
        textures: {}
    };

    for (var j = 0; j < NUM_PANELS; j++) {
        var panelIndex = avatarIndexToPanelIndex(j);
        textureProp["textures"]["file" + panelIndex] = avatars[j].preview_url;
    };

    Overlays.editOverlay(panelWall, textureProp);
}

var MUZAK_VOLUME = 0.1;

function playCurrentSound(secondOffset) {
    if (currentSound == latinSound) {
        if (!latinInjector) {
            latinInjector = Audio.playSound(latinSound, { localOnly: true, secondOffset: secondOffset, volume: MUZAK_VOLUME });
        } else {
            latinInjector.restart();
        }

        currentMuzakInjector = latinInjector;
    } else if (currentSound == elevatorSound) {
        if (!elevatorInjector) {
            elevatorInjector = Audio.playSound(elevatorSound, { localOnly: true, secondOffset: secondOffset, volume: MUZAK_VOLUME });
        } else {
            elevatorInjector.restart();
        }

        currentMuzakInjector = elevatorInjector;
    }
}

function playNextMuzak() {
    if (panelWall) {
        if (currentSound == latinSound) {
            if (elevatorSound.downloaded) {
                currentSound = elevatorSound;
            }
        } else if (currentSound == elevatorSound) {
            if (latinSound.downloaded) {
                currentSound = latinSound;
            }
        }

        playCurrentSound(0);
    }
}

function playRandomMuzak() {
    currentSound = null;

    if (latinSound.downloaded && elevatorSound.downloaded) {
        currentSound = Math.random() < 0.5 ? latinSound : elevatorSound;
    } else if (latinSound.downloaded) {
        currentSound = latinSound;
    } else if (elevatorSound.downloaded) {
        currentSound = elevatorSound;
    }

    if (currentSound) {
        // pick a random number of seconds from 0-10 to offset the muzak
        var secondOffset = Math.random() * 10;

        playCurrentSound(secondOffset);
    } else {
        currentMuzakInjector = null;
    }
}

function cleanupLobby() {
    toggleEnvironmentRendering(true);

    // for each of the 21 placeholder textures, set them back to default so the cached model doesn't have changed textures
    var panelTexturesReset = {};
    panelTexturesReset["textures"] = {};

    for (var j = 0; j < MAX_NUM_PANELS; j++) {
        panelTexturesReset["textures"]["file" + (j + 1)] = LOBBY_BLANK_PANEL_TEXTURE_URL;
    };

    Overlays.editOverlay(panelWall, panelTexturesReset);

    Overlays.deleteOverlay(panelWall);
    Overlays.deleteOverlay(orbShell);
    Overlays.deleteOverlay(descriptionText);

    panelWall = false;
    orbShell = false;

    if (currentDrone) {
        currentDrone.stop();
        currentDrone = null
    }

    if (currentMuzakInjector) {
        currentMuzakInjector.stop();
        currentMuzakInjector = null;
    }

    avatars = {};

}

function actionStartEvent(event) {
    if (panelWall) {
        // we've got an action event and our panel wall is up
        // check if we hit a panel and if we should jump there
        var result = Overlays.findRayIntersection(event.actionRay);
        if (result.intersects && result.overlayID == panelWall) {

            var panelName = result.extraInfo.subMeshName + '';

            var panelStringIndex = panelName.indexOf("Panel");
            if (panelStringIndex != -1) {
                var panelIndex = parseInt(panelName.slice(5));
                var avatarIndex = panelIndexToPlaceIndex(panelIndex);
                if (avatarIndex < avatars.length) {
                    var actionPlace = avatars[avatarIndex];

                    print("Changing avatar to " + actionPlace.name
                      + " after click on panel " + panelIndex + " with avatar index " + avatarIndex);

                    MyAvatar.useFullAvatarURL(actionPlace.content_url);

                    maybeCleanupLobby();
                }
            }
        }
    }
}

var control = false;

function keyPressEvent(event) {
    if (event.text === "CONTROL") {
        control = true;
    }

    if (control && event.text === "a") {
        if (!panelWall) {
            toggleEnvironmentRendering(false);
            drawLobby();
            changeLobbyTextures();
        } else {
            cleanupLobby();
        }
    }
}

function keyReleaseEvent(event) {
    if (event.text === "CONTROL") {
        control = false;
    }
}

var CLEANUP_EPSILON_DISTANCE = 0.05;

function maybeCleanupLobby() {
    if (panelWall && Vec3.length(Vec3.subtract(avatarStickPosition, MyAvatar.position)) > CLEANUP_EPSILON_DISTANCE) {
        cleanupLobby();
    }
}

function toggleEnvironmentRendering(shouldRender) {
    Scene.shouldRenderAvatars = shouldRender;
    Scene.shouldRenderEntities = shouldRender;
}

function handleLookAt(pickRay) {
    if (panelWall && descriptionText) {
        // we've got an action event and our panel wall is up
        // check if we hit a panel and if we should jump there
        var result = Overlays.findRayIntersection(pickRay);
        if (result.intersects && result.overlayID == panelWall) {
            var panelName = result.extraInfo.subMeshName + '';
            var panelStringIndex = panelName.indexOf("Panel");
            if (panelStringIndex != -1) {
                var panelIndex = parseInt(panelName.slice(5));
                var avatarIndex = panelIndexToPlaceIndex(panelIndex);
                if (avatarIndex < avatars.length) {
                    var actionPlace = avatars[avatarIndex];

                    if (actionPlace.description == "") {
                        Overlays.editOverlay(descriptionText, { text: actionPlace.name, visible: showText });
                    } else {
                        // handle line wrapping
                        var allWords = actionPlace.description.split(" ");
                        var currentGoodLine = "";
                        var currentTestLine = "";
                        var formatedDescription = "";
                        var wordsFormated = 0;
                        var currentTestWord = 0;
                        var wordsOnLine = 0;
                        while (wordsFormated < allWords.length) {
                            // first add the "next word" to the line and test it.
                            currentTestLine = currentGoodLine;
                            if (wordsOnLine > 0) {
                                currentTestLine += " " + allWords[currentTestWord];
                            } else {
                                currentTestLine = allWords[currentTestWord];
                            }
                            var lineLength = Overlays.textSize(descriptionText, currentTestLine).width;
                            if (lineLength < textWidth || wordsOnLine == 0) {
                                wordsFormated++;
                                currentTestWord++;
                                wordsOnLine++;
                                currentGoodLine = currentTestLine;
                            } else {
                                formatedDescription += currentGoodLine + "\n";
                                wordsOnLine = 0;
                                currentGoodLine = "";
                                currentTestLine = "";
                            }
                        }
                        formatedDescription += currentGoodLine;
                        Overlays.editOverlay(descriptionText, { text: formatedDescription, visible: showText });
                    }
                } else {
                    Overlays.editOverlay(descriptionText, { text: "", visible: false });
                }
            }
        }
    }
}

function update(deltaTime) {
    maybeCleanupLobby();
    if (panelWall) {
        Overlays.editOverlay(descriptionText, { position: textOverlayPosition() });

        // if the reticle is up then we may need to play the next muzak
        if (currentMuzakInjector && !currentMuzakInjector.playing) {
            playNextMuzak();
        }
    }
}

function mouseMoveEvent(event) {
    if (panelWall) {
        var pickRay = Camera.computePickRay(event.x, event.y);
        handleLookAt(pickRay);
    }
}

Controller.actionStartEvent.connect(actionStartEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.update.connect(update);
Script.scriptEnding.connect(maybeCleanupLobby);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
