//
//  TabletGeneralPreferences.qml
//
//  Created by Dante Ruiz on 9 Feb 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2
import "tabletWindows"
import "../../dialogs"

StackView {
    id: profileRoot
    initialItem: root
    objectName: "stack"
    property string title: "General Settings"
    property alias gotoPreviousApp: root.gotoPreviousApp;
    property alias gotoPreviousAppFromScript: root.gotoPreviousAppFromScript;
    signal sendToScript(var message);

    function pushSource(path) {
        var item = Qt.createComponent(Qt.resolvedUrl(path));
        profileRoot.push(item);
    }

    function popSource() {
        profileRoot.pop();
    }

    function emitSendToScript(message) {
        profileRoot.sendToScript(message);
    }

    TabletPreferencesDialog {
        id: root
        objectName: "TabletGeneralPreferences"
        showCategories: ["User Interface", "Mouse Sensitivity", "HMD", "Snapshots", "Privacy"]
    }
}
