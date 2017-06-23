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

var require = Script.require;

var WANT_DEBUG = false;

var DopplegangerClass = require('./doppleganger.js'),
    DopplegangerAttachments = require('./doppleganger-attachments.js'),
    modelHelper = require('./model-helper.js').modelHelper;

var tablet = Tablet.getTablet('com.highfidelity.interface.tablet.system'),
    button = tablet.addButton({
        icon: Script.resolvePath('./doppleganger-i.svg'),
        activeIcon: Script.resolvePath('./doppleganger-a.svg'),
        text: 'MIRROR'
    });

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
    WANT_DEBUG = true;
    DopplegangerClass.addDebugControls(doppleganger);
}

function debugPrint() {
    if (WANT_DEBUG) {
        print('app-doppleganger | ' + [].slice.call(arguments).join(' '));
    }
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

