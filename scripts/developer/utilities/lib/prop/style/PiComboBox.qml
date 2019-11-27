//
//  Prop/style/PiComboBox.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2


ComboBox {
    id: valueCombo

    height: global.slimHeight
    width: 120
    implicitHeight: global.slimHeight
       

    // look
    flat: true
    delegate: ItemDelegate {
        width: valueCombo.width
        height: valueCombo.height
        contentItem: PiText {
            text: modelData
            horizontalAlignment: global.valueTextAlign
        }
        background: Rectangle {
            color:highlighted ? global.colorBackHighlight : global.colorBack;
        }
        highlighted: valueCombo.highlightedIndex === index
    }

    indicator: PiCanvasIcon {
        id: canvas
        x: valueCombo.width - width - valueCombo.rightPadding
        y: valueCombo.topPadding + (valueCombo.availableHeight - height) / 2

        strokeColor: (valueCombo.down ? global.colorBorderLighter : global.colorBorderLight )
        icon: 1 + valueCombo.down
    }

    contentItem: PiText {
        leftPadding: 0
        rightPadding: valueCombo.indicator.width + valueCombo.spacing

        text: valueCombo.displayText
        horizontalAlignment: global.valueTextAlign
    }

    background: Rectangle {
        color: global.colorBack
        border.color: valueCombo.popup.visible ? global.colorBorderLighter : global.colorBorderLight
        border.width: global.valueBorderWidth
        radius: global.valueBorderRadius
    }

    popup: Popup {
        y: valueCombo.height - 1
        width: valueCombo.width
        implicitHeight: contentItem.implicitHeight + 2
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: valueCombo.popup.visible ? valueCombo.delegateModel : null
            currentIndex: valueCombo.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: global.colorBack
            border.color: global.colorBorderHighight
            radius: global.valueBorderRadius
        }
    }
}    
