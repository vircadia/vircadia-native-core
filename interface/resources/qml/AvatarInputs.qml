//
//  Created by Bradley Austin Davis on 2015/06/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.4
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0

import "./hifi/audio" as HifiAudio

Hifi.AvatarInputs {
    id: root;
    objectName: "AvatarInputs"
    width: audio.width;
    height: audio.height;
    x: 10; y: 5;

    readonly property bool shouldReposition: true;

    HifiAudio.MicBar {
        id: audio;
        visible: root.showAudioTools;
        standalone: true;
	    dragTarget: parent;
    }
}
