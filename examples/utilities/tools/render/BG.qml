//
//  BG.qml
//  examples/utilities/tools/render
//
//  Created by Zach Pomerantz on 2/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    Timer {
        running: true; repeat: true
        onTriggered: time.text = Render.getConfig("DrawBackgroundDeferred").gpuTime
    }

    Text { id: time; font.pointSize: 20 }
}

