//
// Audio.qml
//
// Created by Zach Pomerantz on 6/12/2017
// Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import "../../windows"
import "../audio"

ScrollingWindow {
    resizable: true
    destroyOnHidden: true
    width: 400
    height: 577
    minSize: Qt.vector2d(400, 500)

    Audio { id: audioDialog; width: audio.width }

    id: audio
    objectName: "AudioDialog"
    title: audioDialog.title
}
