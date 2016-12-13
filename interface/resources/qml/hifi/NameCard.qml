//
//  NameCard.qml
//  qml/hifi
//
//  Created by Howard Stearns on 12/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.5
import "../styles-uit"


Column {
    visible: !isCheckBox;
    property string displayName: styleData.value;
    property string userName: model.userName;
    property int displayTextHeight: 18;
    property int usernameTextHeight: 12;

    RalewaySemiBold {
        text: parent.displayName;
        size: parent.displayTextHeight;
        elide: Text.ElideRight;
        width: parent.width;
    }
    RalewayLight {
        visible: styleData.value;
        text: parent.userName;
        size: parent.usernameTextHeight;
        elide: Text.ElideRight;
        width: parent.width;
    }
}
