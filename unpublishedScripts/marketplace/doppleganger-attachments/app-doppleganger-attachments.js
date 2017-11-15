//  doppleganger-app.js
//
//  Created by Timothy Dedischew on 04/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  This Client script creates an instance of a Doppleganger that can be toggled on/off via tablet button.
//  (for more info see doppleganger.js)
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* eslint-env commonjs */
/* global DriveKeys, require:true, console */
/* eslint-disable comma-dangle */

// decomment next line for automatic require cache-busting
// var require = function require(id) { return Script.require(id + '?'+new Date().getTime().toString(36)); };
if (typeof require !== 'function') {
    require = Script.require;
}

var VERSION = '0.0.0';
var WANT_DEBUG = false;
var DEBUG_MOVE_AS_YOU_MOVE = false;
var ROTATE_AS_YOU_MOVE = false;

log(VERSION);

var DopplegangerClass = require('./doppleganger.js'),
    DopplegangerAttachments = require('./doppleganger-attachments.js'),
    DebugControls = require('./doppleganger-debug.js'),
    modelHelper = require('./model-helper.js').modelHelper,
    utils = require('./utils.js');

// eslint-disable-next-line camelcase
var isWebpack = typeof __webpack_require__ === 'function';

var buttonConfig = utils.assign({
    text: 'MIRROR',
}, !isWebpack ? {
    icon: Script.resolvePath('./doppleganger-i.svg'),
    activeIcon: Script.resolvePath('./doppleganger-a.svg'),
} : {
    icon: require('./doppleganger-i.svg.json'),
    activeIcon: require('./doppleganger-a.svg.json'),
});

var tablet = Tablet.getTablet('com.highfidelity.interface.tablet.system'),
    button = tablet.addButton(buttonConfig);

Script.scriptEnding.connect(function() {
    tablet.removeButton(button);
    button = null;
});

var doppleganger = new DopplegangerClass({
    avatar: MyAvatar,
    mirrored: false,
    autoUpdate: true,
    type: 'overlay'
});

// add support for displaying regular (non-soft) attachments on the doppleganger
{
    var RECHECK_ATTACHMENT_MS = 1000;
    var dopplegangerAttachments = new DopplegangerAttachments(doppleganger),
        attachmentInterval = null,
        lastHash = dopplegangerAttachments.getAttachmentsHash();

    // monitor for attachment changes, but only when the doppleganger is active
    doppleganger.activeChanged.connect(function(active, reason) {
        if (attachmentInterval) {
            Script.clearInterval(attachmentInterval);
        }
        if (active) {
            attachmentInterval = Script.setInterval(checkAttachmentsHash, RECHECK_ATTACHMENT_MS);
        } else {
            attachmentInterval = null;
        }
    });
    function checkAttachmentsHash() {
        var currentHash = dopplegangerAttachments.getAttachmentsHash();
        if (currentHash !== lastHash) {
            lastHash = currentHash;
            debugPrint('app-doppleganger | detect attachment change');
            dopplegangerAttachments.refreshAttachments();
        }
    }
}

// add support for "move as you move"
{
    var movementKeys = 'W,A,S,D,Up,Down,Right,Left'.split(',');
    var controllerKeys = 'LX,LY,RY'.split(',');
    var translationKeys = Object.keys(DriveKeys).filter(function(p) {
        return /translate/i.test(p);
    });
    var startingPosition;

    // returns an array of any active driving keys (eg: ['W', 'TRANSLATE_Z'])
    function currentDrivers() {
        return [].concat(
            movementKeys.map(function(key) {
                return Controller.getValue(Controller.Hardware.Keyboard[key]) && key;
            })
        ).concat(
            controllerKeys.map(function(key) {
                return Controller.getValue(Controller.Standard[key]) !== 0.0 && key;
            })
        ).concat(
            translationKeys.map(function(key) {
                return MyAvatar.getRawDriveKey(DriveKeys[key]) !== 0.0 && key;
            })
        ).filter(Boolean);
    }

    doppleganger.jointsUpdated.connect(function(objectID) {
        var drivers = currentDrivers(),
            isDriving = drivers.length > 0;
        if (isDriving) {
            if (startingPosition) {
                debugPrint('resetting startingPosition since drivers == ', drivers.join('|'));
                startingPosition = null;
            }
        } else if (HMD.active || DEBUG_MOVE_AS_YOU_MOVE) {
            startingPosition = startingPosition || MyAvatar.position;
            var movement = Vec3.subtract(MyAvatar.position, startingPosition);
            startingPosition = MyAvatar.position;
            // Vec3.length(movement) > 0.0001 && Vec3.print('+avatarMovement', movement);

            // "mirror" the relative translation vector
            movement.x *= -1;
            movement.z *= -1;
            var props = {};
            props.position = doppleganger.position = Vec3.sum(doppleganger.position, movement);
            if (ROTATE_AS_YOU_MOVE) {
                props.rotation = doppleganger.orientation = MyAvatar.orientation;
            }
            modelHelper.editObject(doppleganger.objectID, props);
        }
    });
}

// hide the doppleganger if this client script is unloaded
Script.scriptEnding.connect(doppleganger, 'stop');

// hide the doppleganger if the user switches domains (which might place them arbitrarily far away in world space)
function onDomainChanged() {
    if (doppleganger.active) {
        doppleganger.stop('domain_changed');
    }
}
Window.domainChanged.connect(onDomainChanged);
Window.domainConnectionRefused.connect(onDomainChanged);
Script.scriptEnding.connect(function() {
    Window.domainChanged.disconnect(onDomainChanged);
    Window.domainConnectionRefused.disconnect(onDomainChanged);
});

// toggle on/off via tablet button
button.clicked.connect(doppleganger, 'toggle');

// highlight tablet button based on current doppleganger state
doppleganger.activeChanged.connect(function(active, reason) {
    if (button) {
        button.editProperties({ isActive: active });
        debugPrint('doppleganger.activeChanged', active, reason);
    }
});

// alert the user if there was an error applying their skeletonModelURL
doppleganger.modelLoaded.connect(function(error, result) {
    if (doppleganger.active && error) {
        Window.alert('doppleganger | ' + error + '\n' + doppleganger.skeletonModelURL);
    }
});

// ----------------------------------------------------------------------------

// add debug indicators, but only if the user has configured the settings value
if (Settings.getValue('debug.doppleganger', false)) {
    WANT_DEBUG = WANT_DEBUG || true;
    DopplegangerClass.WANT_DEBUG = WANT_DEBUG;
    DopplegangerAttachments.WANT_DEBUG = WANT_DEBUG;
    new DebugControls(doppleganger);
}

function log() {
    // eslint-disable-next-line no-console
    (typeof console === 'object' ? console.log : print)('app-doppleganger | ' + [].slice.call(arguments).join(' '));
}

function debugPrint() {
    WANT_DEBUG && log.apply(this, arguments);
}

// ----------------------------------------------------------------------------

UserActivityLogger.logAction('doppleganger_app_load');
doppleganger.activeChanged.connect(function(active, reason) {
    if (active) {
        UserActivityLogger.logAction('doppleganger_enable');
    } else {
        if (reason === 'stop') {
            // user intentionally toggled the doppleganger
            UserActivityLogger.logAction('doppleganger_disable');
        } else {
            debugPrint('doppleganger stopped:', reason);
            UserActivityLogger.logAction('doppleganger_autodisable', { reason: reason });
        }
    }
});
dopplegangerAttachments.attachmentsUpdated.connect(function(attachments) {
    UserActivityLogger.logAction('doppleganger_attachments', { count: attachments.length });
});

