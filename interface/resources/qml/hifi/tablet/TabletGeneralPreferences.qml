//
//  TabletGeneralPreferences.qml
//
//  Created by Dante Ruiz on 9 Feb 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import "tabletWindows"
import "../../dialogs"

StackView {
    id: profileRoot
    initialItem: root
    objectName: "stack"
    property string title: "General Settings"
    property alias gotoPreviousApp: root.gotoPreviousApp;
    signal sendToScript(var message);

    function pushSource(path) {
        profileRoot.push(Qt.resolvedUrl(path));
    }

    function popSource() {
        profileRoot.pop();
    }

    TabletPreferencesDialog {
        id: root
        objectName: "TabletGeneralPreferences"
        showCategories: ["UI", "Snapshots", "Scripts", "Privacy", "Octree", "HMD", "Game Controller", "Sixense Controllers", "Perception Neuron", "Kinect", "Leap Motion"]
    }
}
