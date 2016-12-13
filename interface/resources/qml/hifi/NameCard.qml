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
    property string displayName: "";
    property string userName: "";
    property int displayTextHeight: 18;
    property int usernameTextHeight: 12;

    RalewaySemiBold {
        text: parent.displayName;
        size: parent.displayTextHeight;
        elide: Text.ElideRight;
        width: parent.width;
    }
    RalewayLight {
        visible: parent.displayName;
        text: parent.userName;
        size: parent.usernameTextHeight;
        elide: Text.ElideRight;
        width: parent.width;
    }
}
