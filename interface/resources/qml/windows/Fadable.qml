//
//  Fadable.qml
//
//  Created by Bradley Austin Davis on 15 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0

import "../styles-uit"

// Enable window visibility transitions
FocusScope {
    id: root
    HifiConstants { id: hifi }

    Component.onCompleted: {
        fadeTargetProperty = visible ? 1.0 : 0.0
    }

    property var completionCallback;
    // The target property to animate, usually scale or opacity
    property alias fadeTargetProperty: root.opacity
    // always start the property at 0 to enable fade in on creation
    fadeTargetProperty: 0
    // DO NOT set visible to false or when derived types override it it
    // will short circuit the fade in on initial visibility
    // visible: false <--- NO

    // Some dialogs should be destroyed when they become
    // invisible, so handle that
    onVisibleChanged: {
        // If someone directly set the visibility to false
        // toggle it back on and use the targetVisible flag to transition
        // via fading.
        if ((!visible && fadeTargetProperty != 0.0) || (visible && fadeTargetProperty == 0.0)) {
            var target = visible;
            visible = !visible;
            fadeTargetProperty = target ? 1.0 : 0.0;
            return;
        }

        // Now handle completions
        if (completionCallback) {
            completionCallback();
            completionCallback = undefined;
        }

    }

    // The actual animator
    Behavior on fadeTargetProperty {
        NumberAnimation {
            duration: hifi.effects.fadeInDuration
            easing.type: Easing.InOutCubic
        }
    }

    onFadeTargetPropertyChanged: {
        visible = (fadeTargetProperty != 0.0);
    }

    function fadeIn(callback) {
        completionCallback = callback;
        fadeTargetProperty = 1.0;
    }

    function fadeOut(callback) {
        completionCallback = callback;
        fadeTargetProperty = 0.0;
    }
}
