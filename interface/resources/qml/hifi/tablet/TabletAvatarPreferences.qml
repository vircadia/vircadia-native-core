//
//  TabletAvatarPreferences.qml
//
//  Created by Davd Rowe on 2 Mar 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import "tabletWindows"
import "../../dialogs"
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0

StackView {
    id: profileRoot
    initialItem: root
    objectName: "stack"

    property var eventBridge;
    signal sendToScript(var message);

    function pushSource(path) {
        editRoot.push(Qt.reslovedUrl(path));
    }

    function popSource() {

    }

    TabletPreferencesDialog {
        id: root
        objectName: "TabletAvatarPreferences"
        width: parent.width
        height: parent.height
        showCategories: ["Avatar Basics", "Avatar Tuning", "Avatar Camera"]
    }
}
