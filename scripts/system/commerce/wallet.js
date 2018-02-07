"use strict";
/*jslint vars:true, plusplus:true, forin:true*/
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// wallet.js
//
// Created by Zach Fox on 2017-08-17
// Copyright 2017 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/*global XXX */

(function () { // BEGIN LOCAL_SCOPE
    Script.include("/~/system/libraries/accountUtils.js");
    var request = Script.require('request').request;

    var MARKETPLACE_URL = Account.metaverseServerURL + "/marketplace";

    // Function Name: onButtonClicked()
    //
    // Description:
    //   -Fired when the app button is pressed.
    //
    // Relevant Variables:
    //   -WALLET_QML_SOURCE: The path to the Wallet QML
    //   -onWalletScreen: true/false depending on whether we're looking at the app.
    var WALLET_QML_SOURCE = "hifi/commerce/wallet/Wallet.qml";
    var MARKETPLACE_PURCHASES_QML_PATH = "hifi/commerce/purchases/Purchases.qml";
    var onWalletScreen = false;
    function onButtonClicked() {
        if (!tablet) {
            print("Warning in buttonClicked(): 'tablet' undefined!");
            return;
        }
        if (onWalletScreen) {
            // for toolbar-mode: go back to home screen, this will close the window.
            tablet.gotoHomeScreen();
        } else {
            tablet.loadQMLSource(WALLET_QML_SOURCE);
        }
    }

    // Function Name: sendToQml()
    //
    // Description:
    //   -Use this function to send a message to the QML (i.e. to change appearances). The "message" argument is what is sent to
    //    the QML in the format "{method, params}", like json-rpc. See also fromQml().
    function sendToQml(message) {
        tablet.sendToQml(message);
    }

    //***********************************************
    //
    // BEGIN Connection logic
    //
    //***********************************************
    // Function Names:
    //   - requestJSON
    //   - getAvailableConnections
    //   - getInfoAboutUser
    //   - getConnectionData
    //
    // Description:
    //   - Update all the usernames that I am entitled to see, using my login but not dependent on canKick.
    var METAVERSE_BASE = Account.metaverseServerURL;
    function requestJSON(url, callback) { // callback(data) if successfull. Logs otherwise.
        request({
            uri: url
        }, function (error, response) {
            if (error || (response.status !== 'success')) {
                print("Error: unable to get", url, error || response.status);
                return;
            }
            callback(response.data);
        });
    }
    function getAvailableConnections(domain, callback) { // callback([{usename, location}...]) if successful. (Logs otherwise)
        url = METAVERSE_BASE + '/api/v1/users?per_page=400&'
        if (domain) {
            url += 'status=' + domain.slice(1, -1); // without curly braces
        } else {
            url += 'filter=connections'; // regardless of whether online
        }
        requestJSON(url, function (connectionsData) {
            callback(connectionsData.users);
        });
    }
    function getInfoAboutUser(specificUsername, callback) {
        url = METAVERSE_BASE + '/api/v1/users?filter=connections'
        requestJSON(url, function (connectionsData) {
            for (user in connectionsData.users) {
                if (connectionsData.users[user].username === specificUsername) {
                    callback(connectionsData.users[user]);
                    return;
                }
            }
            callback(false);
        });
    }
    function getConnectionData(specificUsername, domain) { 
        function frob(user) { // get into the right format
            var formattedSessionId = user.location.node_id || '';
            if (formattedSessionId !== '' && formattedSessionId.indexOf("{") != 0) {
                formattedSessionId = "{" + formattedSessionId + "}";
            }
            return {
                sessionId: formattedSessionId,
                userName: user.username,
                connection: user.connection,
                profileUrl: user.images.thumbnail,
                placeName: (user.location.root || user.location.domain || {}).name || ''
            };
        }
        if (specificUsername) {
            getInfoAboutUser(specificUsername, function (user) {
                if (user) {
                    updateUser(frob(user));
                } else {
                    print('Error: Unable to find information about ' + specificUsername + ' in connectionsData!');
                }
            });
        } else {
            getAvailableConnections(domain, function (users) {
                if (domain) {
                    users.forEach(function (user) {
                        updateUser(frob(user));
                    });
                } else {
                    sendToQml({ method: 'updateConnections', connections: users.map(frob) });
                }
            });
        }
    }
    //***********************************************
    //
    // END Connection logic
    //
    //***********************************************

    //***********************************************
    //
    // BEGIN Avatar Selector logic
    //
    //***********************************************
    var UNSELECTED_TEXTURES = {
        "idle-D": Script.resolvePath("./assets/models/Avatar-Overlay-v1.fbx/Avatar-Overlay-v1.fbm/avatar-overlay-idle.png"),
        "idle-E": Script.resolvePath("./assets/models/Avatar-Overlay-v1.fbx/Avatar-Overlay-v1.fbm/avatar-overlay-idle.png")
    };
    var SELECTED_TEXTURES = {
        "idle-D": Script.resolvePath("./assets/models/Avatar-Overlay-v1.fbx/Avatar-Overlay-v1.fbm/avatar-overlay-selected.png"),
        "idle-E": Script.resolvePath("./assets/models/Avatar-Overlay-v1.fbx/Avatar-Overlay-v1.fbm/avatar-overlay-selected.png")
    };
    var HOVER_TEXTURES = {
        "idle-D": Script.resolvePath("./assets/models/Avatar-Overlay-v1.fbx/Avatar-Overlay-v1.fbm/avatar-overlay-hover.png"),
        "idle-E": Script.resolvePath("./assets/models/Avatar-Overlay-v1.fbx/Avatar-Overlay-v1.fbm/avatar-overlay-hover.png")
    };

    var UNSELECTED_COLOR = { red: 0x1F, green: 0xC6, blue: 0xA6 };
    var SELECTED_COLOR = { red: 0xF3, green: 0x91, blue: 0x29 };
    var HOVER_COLOR = { red: 0xD0, green: 0xD0, blue: 0xD0 };
    var conserveResources = true;

    var overlays = {}; // Keeps track of all our extended overlay data objects, keyed by target identifier.

    function ExtendedOverlay(key, type, properties, selected, hasModel) { // A wrapper around overlays to store the key it is associated with.
        overlays[key] = this;
        if (hasModel) {
            var modelKey = key + "-m";
            this.model = new ExtendedOverlay(modelKey, "model", {
                url: Script.resolvePath("./assets/models/Avatar-Overlay-v1.fbx"),
                textures: textures(selected),
                ignoreRayIntersection: true
            }, false, false);
        } else {
            this.model = undefined;
        }
        this.key = key;
        this.selected = selected || false; // not undefined
        this.hovering = false;
        this.activeOverlay = Overlays.addOverlay(type, properties); // We could use different overlays for (un)selected...
    }
    // Instance methods:
    ExtendedOverlay.prototype.deleteOverlay = function () { // remove display and data of this overlay
        Overlays.deleteOverlay(this.activeOverlay);
        delete overlays[this.key];
    };

    ExtendedOverlay.prototype.editOverlay = function (properties) { // change display of this overlay
        Overlays.editOverlay(this.activeOverlay, properties);
    };

    function color(selected, hovering) {
        var base = hovering ? HOVER_COLOR : selected ? SELECTED_COLOR : UNSELECTED_COLOR;
        function scale(component) {
            var delta = 0xFF - component;
            return component;
        }
        return { red: scale(base.red), green: scale(base.green), blue: scale(base.blue) };
    }

    function textures(selected, hovering) {
        return hovering ? HOVER_TEXTURES : selected ? SELECTED_TEXTURES : UNSELECTED_TEXTURES;
    }
    // so we don't have to traverse the overlays to get the last one
    var lastHoveringId = 0;
    ExtendedOverlay.prototype.hover = function (hovering) {
        this.hovering = hovering;
        if (this.key === lastHoveringId) {
            if (hovering) {
                return;
            }
            lastHoveringId = 0;
        }
        this.editOverlay({ color: color(this.selected, hovering) });
        if (this.model) {
            this.model.editOverlay({ textures: textures(this.selected, hovering) });
        }
        if (hovering) {
            // un-hover the last hovering overlay
            if (lastHoveringId && lastHoveringId !== this.key) {
                ExtendedOverlay.get(lastHoveringId).hover(false);
            }
            lastHoveringId = this.key;
        }
    };
    ExtendedOverlay.prototype.select = function (selected) {
        if (this.selected === selected) {
            return;
        }

        this.editOverlay({ color: color(selected, this.hovering) });
        if (this.model) {
            this.model.editOverlay({ textures: textures(selected) });
        }
        this.selected = selected;
    };
    // Class methods:
    var selectedIds = [];
    ExtendedOverlay.isSelected = function (id) {
        return -1 !== selectedIds.indexOf(id);
    };
    ExtendedOverlay.get = function (key) { // answer the extended overlay data object associated with the given avatar identifier
        return overlays[key];
    };
    ExtendedOverlay.some = function (iterator) { // Bails early as soon as iterator returns truthy.
        var key;
        for (key in overlays) {
            if (iterator(ExtendedOverlay.get(key))) {
                return;
            }
        }
    };
    ExtendedOverlay.unHover = function () { // calls hover(false) on lastHoveringId (if any)
        if (lastHoveringId) {
            ExtendedOverlay.get(lastHoveringId).hover(false);
        }
    };

    // hit(overlay) on the one overlay intersected by pickRay, if any.
    // noHit() if no ExtendedOverlay was intersected (helps with hover)
    ExtendedOverlay.applyPickRay = function (pickRay, hit, noHit) {
        var pickedOverlay = Overlays.findRayIntersection(pickRay); // Depends on nearer coverOverlays to extend closer to us than farther ones.
        if (!pickedOverlay.intersects) {
            if (noHit) {
                return noHit();
            }
            return;
        }
        ExtendedOverlay.some(function (overlay) { // See if pickedOverlay is one of ours.
            if ((overlay.activeOverlay) === pickedOverlay.overlayID) {
                hit(overlay);
                return true;
            }
        });
    };

    function HighlightedEntity(id, entityProperties) {
        this.id = id;
        this.overlay = Overlays.addOverlay('cube', {
            position: entityProperties.position,
            rotation: entityProperties.rotation,
            dimensions: entityProperties.dimensions,
            solid: false,
            color: {
                red: 0xF3,
                green: 0x91,
                blue: 0x29
            },
            ignoreRayIntersection: true,
            drawInFront: false // Arguable. For now, let's not distract with mysterious wires around the scene.
        });
        HighlightedEntity.overlays.push(this);
    }
    HighlightedEntity.overlays = [];
    HighlightedEntity.clearOverlays = function clearHighlightedEntities() {
        HighlightedEntity.overlays.forEach(function (highlighted) {
            Overlays.deleteOverlay(highlighted.overlay);
        });
        HighlightedEntity.overlays = [];
    };
    HighlightedEntity.updateOverlays = function updateHighlightedEntities() {
        HighlightedEntity.overlays.forEach(function (highlighted) {
            var properties = Entities.getEntityProperties(highlighted.id, ['position', 'rotation', 'dimensions']);
            Overlays.editOverlay(highlighted.overlay, {
                position: properties.position,
                rotation: properties.rotation,
                dimensions: properties.dimensions
            });
        });
    };


    function addAvatarNode(id) {
        var selected = ExtendedOverlay.isSelected(id);
        return new ExtendedOverlay(id, "sphere", {
            drawInFront: true,
            solid: true,
            alpha: 0.8,
            color: color(selected, false),
            ignoreRayIntersection: false
        }, selected, !conserveResources);
    }

    var pingPong = true;
    function updateOverlays() {
        var eye = Camera.position;
        AvatarList.getAvatarIdentifiers().forEach(function (id) {
            if (!id) {
                return; // don't update ourself, or avatars we're not interested in
            }
            var avatar = AvatarList.getAvatar(id);
            if (!avatar) {
                return; // will be deleted below if there had been an overlay.
            }
            var overlay = ExtendedOverlay.get(id);
            if (!overlay) { // For now, we're treating this as a temporary loss, as from the personal space bubble. Add it back.
                overlay = addAvatarNode(id);
            }
            var target = avatar.position;
            var distance = Vec3.distance(target, eye);
            var offset = 0.2;
            var diff = Vec3.subtract(target, eye); // get diff between target and eye (a vector pointing to the eye from avatar position)
            var headIndex = avatar.getJointIndex("Head"); // base offset on 1/2 distance from hips to head if we can
            if (headIndex > 0) {
                offset = avatar.getAbsoluteJointTranslationInObjectFrame(headIndex).y / 2;
            }

            // move a bit in front, towards the camera
            target = Vec3.subtract(target, Vec3.multiply(Vec3.normalize(diff), offset));

            // now bump it up a bit
            target.y = target.y + offset;

            overlay.ping = pingPong;
            overlay.editOverlay({
                color: color(ExtendedOverlay.isSelected(id), overlay.hovering),
                position: target,
                dimensions: 0.032 * distance
            });
            if (overlay.model) {
                overlay.model.ping = pingPong;
                overlay.model.editOverlay({
                    position: target,
                    scale: 0.2 * distance, // constant apparent size
                    rotation: Camera.orientation
                });
            }
        });
        pingPong = !pingPong;
        ExtendedOverlay.some(function (overlay) { // Remove any that weren't updated. (User is gone.)
            if (overlay.ping === pingPong) {
                overlay.deleteOverlay();
            }
        });
        // We could re-populateNearbyUserList if anything added or removed, but not for now.
        HighlightedEntity.updateOverlays();
    }
    function removeOverlays() {
        selectedIds = [];
        lastHoveringId = 0;
        HighlightedEntity.clearOverlays();
        ExtendedOverlay.some(function (overlay) {
            overlay.deleteOverlay();
        });
    }

    //
    // Clicks.
    //
    function usernameFromIDReply(id, username, machineFingerprint, isAdmin) {
        if (selectedIds[0] === id) {
            var message = {
                method: 'updateSelectedRecipientUsername',
                userName: username === "" ? "unknown username" : username
            };
            sendToQml(message);
        }
    }
    function handleClick(pickRay) {
        ExtendedOverlay.applyPickRay(pickRay, function (overlay) {
            var nextSelectedStatus = !overlay.selected;
            var avatarId = overlay.key;
            selectedIds = nextSelectedStatus ? [avatarId] : [];
            if (nextSelectedStatus) {
                Users.requestUsernameFromID(avatarId);
            }
            var message = {
                method: 'selectRecipient',
                id: [avatarId],
                isSelected: nextSelectedStatus,
                displayName: '"' + AvatarList.getAvatar(avatarId).sessionDisplayName + '"',
                userName: ''
            };
            sendToQml(message);
            
            ExtendedOverlay.some(function (overlay) {
                var id = overlay.key;
                var selected = ExtendedOverlay.isSelected(id);
                overlay.select(selected);
            });

            HighlightedEntity.clearOverlays();
            if (selectedIds.length) {
                Entities.findEntitiesInFrustum(Camera.frustum).forEach(function (id) {
                    // Because lastEditedBy is per session, the vast majority of entities won't match,
                    // so it would probably be worth reducing marshalling costs by asking for just we need.
                    // However, providing property name(s) is advisory and some additional properties are
                    // included anyway. As it turns out, asking for 'lastEditedBy' gives 'position', 'rotation',
                    // and 'dimensions', too, so we might as well make use of them instead of making a second
                    // getEntityProperties call.
                    // It would be nice if we could harden this against future changes by specifying all
                    // and only these four in an array, but see
                    // https://highfidelity.fogbugz.com/f/cases/2728/Entities-getEntityProperties-id-lastEditedBy-name-lastEditedBy-doesn-t-work
                    var properties = Entities.getEntityProperties(id, 'lastEditedBy');
                    if (ExtendedOverlay.isSelected(properties.lastEditedBy)) {
                        new HighlightedEntity(id, properties);
                    }
                });
            }
            return true;
        });
    }
    function handleMouseEvent(mousePressEvent) { // handleClick if we get one.
        if (!mousePressEvent.isLeftButton) {
            return;
        }
        handleClick(Camera.computePickRay(mousePressEvent.x, mousePressEvent.y));
    }
    function handleMouseMove(pickRay) { // given the pickRay, just do the hover logic
        ExtendedOverlay.applyPickRay(pickRay, function (overlay) {
            overlay.hover(true);
        }, function () {
            ExtendedOverlay.unHover();
        });
    }

    // handy global to keep track of which hand is the mouse (if any)
    var currentHandPressed = 0;
    var TRIGGER_CLICK_THRESHOLD = 0.85;
    var TRIGGER_PRESS_THRESHOLD = 0.05;

    function handleMouseMoveEvent(event) { // find out which overlay (if any) is over the mouse position
        var pickRay;
        if (HMD.active) {
            if (currentHandPressed !== 0) {
                pickRay = controllerComputePickRay(currentHandPressed);
            } else {
                // nothing should hover, so
                ExtendedOverlay.unHover();
                return;
            }
        } else {
            pickRay = Camera.computePickRay(event.x, event.y);
        }
        handleMouseMove(pickRay);
    }
    function handleTriggerPressed(hand, value) {
        // The idea is if you press one trigger, it is the one
        // we will consider the mouse.  Even if the other is pressed,
        // we ignore it until this one is no longer pressed.
        var isPressed = value > TRIGGER_PRESS_THRESHOLD;
        if (currentHandPressed === 0) {
            currentHandPressed = isPressed ? hand : 0;
            return;
        }
        if (currentHandPressed === hand) {
            currentHandPressed = isPressed ? hand : 0;
            return;
        }
        // otherwise, the other hand is still triggered
        // so do nothing.
    }

    // We get mouseMoveEvents from the handControllers, via handControllerPointer.
    // But we don't get mousePressEvents.
    var triggerMapping = Controller.newMapping(Script.resolvePath('') + '-click');
    var triggerPressMapping = Controller.newMapping(Script.resolvePath('') + '-press');
    function controllerComputePickRay(hand) {
        var controllerPose = getControllerWorldLocation(hand, true);
        if (controllerPose.valid) {
            return { origin: controllerPose.position, direction: Quat.getUp(controllerPose.orientation) };
        }
    }
    function makeClickHandler(hand) {
        return function (clicked) {
            if (clicked > TRIGGER_CLICK_THRESHOLD) {
                var pickRay = controllerComputePickRay(hand);
                handleClick(pickRay);
            }
        };
    }
    function makePressHandler(hand) {
        return function (value) {
            handleTriggerPressed(hand, value);
        };
    }
    triggerMapping.from(Controller.Standard.RTClick).peek().to(makeClickHandler(Controller.Standard.RightHand));
    triggerMapping.from(Controller.Standard.LTClick).peek().to(makeClickHandler(Controller.Standard.LeftHand));
    triggerPressMapping.from(Controller.Standard.RT).peek().to(makePressHandler(Controller.Standard.RightHand));
    triggerPressMapping.from(Controller.Standard.LT).peek().to(makePressHandler(Controller.Standard.LeftHand));
    //***********************************************
    //
    // END Avatar Selector logic
    //
    //***********************************************

    var sendMoneyRecipient;
    var sendMoneyParticleEffectUpdateTimer;
    var particleEffectTimestamp;
    var sendMoneyParticleEffect;
    var SEND_MONEY_PARTICLE_TIMER_UPDATE = 250;
    var SEND_MONEY_PARTICLE_EMITTING_DURATION = 3000;
    var SEND_MONEY_PARTICLE_LIFETIME_SECONDS = 8;
    var SEND_MONEY_PARTICLE_PROPERTIES = {
        accelerationSpread: { x: 0, y: 0, z: 0 },
        alpha: 1,
        alphaFinish: 1,
        alphaSpread: 0,
        alphaStart: 1,
        azimuthFinish: 0,
        azimuthStart: -6,
        color: { red: 143, green: 5, blue: 255 },
        colorFinish: { red: 255, green: 0, blue: 204 },
        colorSpread: { red: 0, green: 0, blue: 0 },
        colorStart: { red: 0, green: 136, blue: 255 },
        emitAcceleration: { x: 0, y: 0, z: 0 }, // Immediately gets updated to be accurate
        emitDimensions: { x: 0, y: 0, z: 0 },
        emitOrientation: { x: 0, y: 0, z: 0 },
        emitRate: 4,
        emitSpeed: 2.1,
        emitterShouldTrail: true,
        isEmitting: 1,
        lifespan: SEND_MONEY_PARTICLE_LIFETIME_SECONDS + 1, // Immediately gets updated to be accurate
        lifetime: SEND_MONEY_PARTICLE_LIFETIME_SECONDS + 1,
        maxParticles: 20,
        name: 'hfc-particles',
        particleRadius: 0.2,
        polarFinish: 0,
        polarStart: 0,
        radiusFinish: 0.05,
        radiusSpread: 0,
        radiusStart: 0.2,
        speedSpread: 0,
        textures: "http://hifi-content.s3.amazonaws.com/alan/dev/Particles/Bokeh-Particle-HFC.png",
        type: 'ParticleEffect'
    };

    function updateSendMoneyParticleEffect() {
        var timestampNow = Date.now();
        if ((timestampNow - particleEffectTimestamp) > (SEND_MONEY_PARTICLE_LIFETIME_SECONDS * 1000)) {
            deleteSendMoneyParticleEffect();
            return;
        } else if ((timestampNow - particleEffectTimestamp) > SEND_MONEY_PARTICLE_EMITTING_DURATION) {
            Entities.editEntity(sendMoneyParticleEffect, {
                isEmitting: 0
            });
        } else if (sendMoneyParticleEffect) {
            var recipientPosition = AvatarList.getAvatar(sendMoneyRecipient).position;
            var distance = Vec3.distance(recipientPosition, MyAvatar.position);
            var accel = Vec3.subtract(recipientPosition, MyAvatar.position);
            accel.y -= 3.0;
            var life = Math.sqrt(2 * distance / Vec3.length(accel));
            Entities.editEntity(sendMoneyParticleEffect, {
                emitAcceleration: accel,
                lifespan: life
            });
        }
    }

    function deleteSendMoneyParticleEffect() {
        if (sendMoneyParticleEffectUpdateTimer) {
            Script.clearInterval(sendMoneyParticleEffectUpdateTimer);
            sendMoneyParticleEffectUpdateTimer = null;
        }
        if (sendMoneyParticleEffect) {
            sendMoneyParticleEffect = Entities.deleteEntity(sendMoneyParticleEffect);
        }
        sendMoneyRecipient = null;
    }

    // Function Name: fromQml()
    //
    // Description:
    //   -Called when a message is received from SpectatorCamera.qml. The "message" argument is what is sent from the QML
    //    in the format "{method, params}", like json-rpc. See also sendToQml().
    var isHmdPreviewDisabled = true;
    var MARKETPLACES_INJECT_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesInject.js");

    function fromQml(message) {
        switch (message.method) {
            case 'passphrasePopup_cancelClicked':
            case 'needsLogIn_cancelClicked':
                tablet.gotoHomeScreen();
                break;
            case 'walletSetup_cancelClicked':
                switch (message.referrer) {
                    case '': // User clicked "Wallet" app
                    case undefined:
                    case null:
                        tablet.gotoHomeScreen();
                        break;
                    case 'purchases':
                    case 'marketplace cta':
                    case 'mainPage':
                        tablet.gotoWebScreen(MARKETPLACE_URL, MARKETPLACES_INJECT_SCRIPT_URL);
                        break;
                    default: // User needs to return to an individual marketplace item URL
                        tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + message.referrer, MARKETPLACES_INJECT_SCRIPT_URL);
                        break;
                }
                break;
            case 'needsLogIn_loginClicked':
                openLoginWindow();
                break;
            case 'disableHmdPreview':
                isHmdPreviewDisabled = Menu.isOptionChecked("Disable Preview");
                DesktopPreviewProvider.setPreviewDisabledReason("SECURE_SCREEN");
                Menu.setIsOptionChecked("Disable Preview", true);
                break;
            case 'maybeEnableHmdPreview':
                DesktopPreviewProvider.setPreviewDisabledReason("USER");
                Menu.setIsOptionChecked("Disable Preview", isHmdPreviewDisabled);
                break;
            case 'passphraseReset':
                onButtonClicked();
                onButtonClicked();
                break;
            case 'walletReset':
                Settings.setValue("isFirstUseOfPurchases", true);
                onButtonClicked();
                onButtonClicked();
                break;
            case 'transactionHistory_linkClicked':
                tablet.gotoWebScreen(message.marketplaceLink, MARKETPLACES_INJECT_SCRIPT_URL);
                break;
            case 'goToPurchases_fromWalletHome':
            case 'goToPurchases':
                tablet.pushOntoStack(MARKETPLACE_PURCHASES_QML_PATH);
                break;
            case 'goToMarketplaceItemPage':
                tablet.gotoWebScreen(MARKETPLACE_URL + '/items/' + message.itemId, MARKETPLACES_INJECT_SCRIPT_URL);
                break;
            case 'refreshConnections':
                print('Refreshing Connections...');
                getConnectionData(false);
                break;
            case 'enable_ChooseRecipientNearbyMode':
                if (!isUpdateOverlaysWired) {
                    Script.update.connect(updateOverlays);
                    isUpdateOverlaysWired = true;
                }
                break;
            case 'disable_ChooseRecipientNearbyMode':
                if (isUpdateOverlaysWired) {
                    Script.update.disconnect(updateOverlays);
                    isUpdateOverlaysWired = false;
                }
                removeOverlays();
                break;
            case 'sendMoney_sendPublicly':
                deleteSendMoneyParticleEffect();
                sendMoneyRecipient = message.recipient;
                var amount = message.amount;
                var props = SEND_MONEY_PARTICLE_PROPERTIES;
                props.parentID = MyAvatar.sessionUUID;
                props.position = MyAvatar.position;
                props.position.y += 0.2;
                sendMoneyParticleEffect = Entities.addEntity(props, true);
                particleEffectTimestamp = Date.now();
                updateSendMoneyParticleEffect();
                sendMoneyParticleEffectUpdateTimer = Script.setInterval(updateSendMoneyParticleEffect, SEND_MONEY_PARTICLE_TIMER_UPDATE);
                break;
            default:
                print('Unrecognized message from QML:', JSON.stringify(message));
        }
    }

    // Function Name: wireEventBridge()
    //
    // Description:
    //   -Used to connect/disconnect the script's response to the tablet's "fromQml" signal. Set the "on" argument to enable or
    //    disable to event bridge.
    //
    // Relevant Variables:
    //   -hasEventBridge: true/false depending on whether we've already connected the event bridge.
    var hasEventBridge = false;
    function wireEventBridge(on) {
        if (!tablet) {
            print("Warning in wireEventBridge(): 'tablet' undefined!");
            return;
        }
        if (on) {
            if (!hasEventBridge) {
                tablet.fromQml.connect(fromQml);
                hasEventBridge = true;
            }
        } else {
            if (hasEventBridge) {
                tablet.fromQml.disconnect(fromQml);
                hasEventBridge = false;
            }
        }
    }

    // Function Name: onTabletScreenChanged()
    //
    // Description:
    //   -Called when the TabletScriptingInterface::screenChanged() signal is emitted. The "type" argument can be either the string
    //    value of "Home", "Web", "Menu", "QML", or "Closed". The "url" argument is only valid for Web and QML.
    function onTabletScreenChanged(type, url) {
        var onWalletScreenNow = (type === "QML" && url === WALLET_QML_SOURCE);
        if (!onWalletScreenNow && onWalletScreen) {
            DesktopPreviewProvider.setPreviewDisabledReason("USER");
        }
        onWalletScreen = onWalletScreenNow;
        wireEventBridge(onWalletScreen);
        // Change button to active when window is first openend, false otherwise.
        if (button) {
            button.editProperties({ isActive: onWalletScreen });
        }

        if (onWalletScreen) {
            isWired = true;
            Users.usernameFromIDReply.connect(usernameFromIDReply);
            Controller.mousePressEvent.connect(handleMouseEvent);
            Controller.mouseMoveEvent.connect(handleMouseMoveEvent);
            triggerMapping.enable();
            triggerPressMapping.enable();
        } else {
            off();
        }
    }

    //
    // Manage the connection between the button and the window.
    //
    var button;
    var buttonName = "WALLET";
    var tablet = null;
    var walletEnabled = Settings.getValue("commerce", true);
    function startup() {
        if (walletEnabled) {
            tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
            button = tablet.addButton({
                text: buttonName,
                icon: "icons/tablet-icons/wallet-i.svg",
                activeIcon: "icons/tablet-icons/wallet-a.svg",
                sortOrder: 10
            });
            button.clicked.connect(onButtonClicked);
            tablet.screenChanged.connect(onTabletScreenChanged);
        }
    }
    var isWired = false;
    var isUpdateOverlaysWired = false;
    function off() {
        if (isWired) { // It is not ok to disconnect these twice, hence guard.
            Users.usernameFromIDReply.disconnect(usernameFromIDReply);
            Controller.mousePressEvent.disconnect(handleMouseEvent);
            Controller.mouseMoveEvent.disconnect(handleMouseMoveEvent);
            isWired = false;
        }
        if (isUpdateOverlaysWired) {
            Script.update.disconnect(updateOverlays);
            isUpdateOverlaysWired = false;
        }
        triggerMapping.disable(); // It's ok if we disable twice.
        triggerPressMapping.disable(); // see above
        removeOverlays();
    }
    function shutdown() {
        button.clicked.disconnect(onButtonClicked);
        tablet.removeButton(button);
        deleteSendMoneyParticleEffect();
        if (tablet) {
            tablet.screenChanged.disconnect(onTabletScreenChanged);
            if (onWalletScreen) {
                tablet.gotoHomeScreen();
            }
        }
        off();
    }

    //
    // Run the functions.
    //
    startup();
    Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
