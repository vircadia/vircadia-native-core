//
//  QueuedButton.qml
//  -- original Button.qml + signal timer workaround --ht
//  Created by David Rowe on 16 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "../styles-uit"
import "." as HifiControls

HifiControls.Button {
    // FIXME: THIS WORKAROUND MIGRATED/CONSOLIDATED FROM RUNNINGSCRIPTS.QML

    // For some reason trigginer an API that enters
    // an internal event loop directly from the button clicked
    // trigger below causes the appliction to behave oddly.
    // Most likely because the button onClicked handling is never
    // completed until the function returns.
    // FIXME find a better way of handling the input dialogs that
    // doesn't trigger this.

    // NOTE: dialogs that need to use this workaround can connect via
    //    onQueuedClicked: ...
    // instead of:
    //    onClicked: ...

    signal clickedQueued()
    Timer {
        id: fromTimer
        interval: 5
        repeat: false
        running: false
        onTriggered: clickedQueued()
    }
    onClicked: fromTimer.running = true
}
