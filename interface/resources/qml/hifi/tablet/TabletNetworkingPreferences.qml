//
//  TabletNetworkingPreferences.qml
//
//  Created by Davd Rowe on 7 Mar 2017.
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
    property var title: "Networking Settings"

    signal sendToScript(var message);

    function pushSource(path) {
        profileRoot.push(Qt.resolvedUrl(path));
    }

    function popSource() {
        profileRoot.pop();
    }

    TabletPreferencesDialog {
        id: root
        objectName: "TabletNetworkingPreferences"
        showCategories: ["Networking"]
    }
}
