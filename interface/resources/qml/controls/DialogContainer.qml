//
//  DialogCommon.qml
//
//  Created by David Rowe on 3 Jun 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import "../styles"

Item {
    id: root

    property bool destroyOnInvisible: true


    // The UI enables an object, rather than manipulating its visibility, so that we can do animations in both directions.
    // Because visibility and enabled are booleans, they cannot be animated. So when enabled is changed, we modify a property
    // that can be animated, like scale or opacity, and then when the target animation value is reached, we can modify the
    // visibility.
    enabled: false
    opacity: 0.0

    onEnabledChanged: {
        opacity = enabled ? 1.0 : 0.0
    }

    Behavior on opacity {
        // Animate opacity.
        NumberAnimation {
            duration: hifi.effects.fadeInDuration
            easing.type: Easing.OutCubic
        }
    }

    onOpacityChanged: {
        // Once we're transparent, disable the dialog's visibility.
        visible = (opacity != 0.0)
    }

    onVisibleChanged: {
        if (!visible) {
            // Some dialogs should be destroyed when they become invisible.
            if (destroyOnInvisible) {
                destroy()
            }
        }
    }


    Keys.onPressed: {
        switch(event.key) {
            case Qt.Key_W:
                if (event.modifiers == Qt.ControlModifier) {
                    enabled = false
                    event.accepted = true
                }
                break
        }
    }
}
