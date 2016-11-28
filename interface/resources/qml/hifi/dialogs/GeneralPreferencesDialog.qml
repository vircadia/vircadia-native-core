//
//  PreferencesDialog.qml
//
//  Created by Bradley Austin Davis on 24 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import Qt.labs.settings 1.0

import "../../dialogs"

PreferencesDialog {
    id: root
    objectName: "GeneralPreferencesDialog"
    title: "General Settings"
    showCategories: ["UI", "Snapshots", "Scripts", "Privacy", "Octree", "HMD", "Sixense Controllers", "Perception Neuron"]
    property var settings: Settings {
        category: root.objectName
        property alias x: root.x
        property alias y: root.y
        property alias width: root.width
        property alias height: root.height
    }
}

