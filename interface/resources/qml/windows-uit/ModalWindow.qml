//
//  ModalWindow.qml
//
//  Created by Bradley Austin Davis on 22 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "."

Window {
    id: window
    anchors.centerIn: parent
    modality: Qt.ApplicationModal
    destroyOnCloseButton: true
    destroyOnInvisible: true
    frame: ModalFrame{}
}
