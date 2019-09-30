//
//  PropEnum.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2

PropItem {
    Global { id: global }
    id: root

    property alias enums : valueCombo.model
    property alias currentIndex : valueCombo.currentIndex

    PropComboBox {
        id: valueCombo

        flat: true

        anchors.left: root.splitter.right
        anchors.right: root.right 
        anchors.verticalCenter: root.verticalCenter
        height: global.slimHeight

        currentIndex: root.valueVarGetter()
        onCurrentIndexChanged: root.valueVarSetter(currentIndex)
    }    
}
