//
//  TextEdit.qml
//
//  Created by Bradley Austin Davis on 24 Apr 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "../styles-uit"

TextEdit {

    property real size: 32

    FontLoader { id: ralewaySemiBold; source: "../../fonts/Raleway-SemiBold.ttf"; }
    font.family: ralewaySemiBold.name
    font.pointSize: size
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft
}
