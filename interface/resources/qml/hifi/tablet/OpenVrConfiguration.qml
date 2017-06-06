//
//  Created by Dante Ruiz on 6/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtGraphicalEffects 1.0
import "../../styles-uit"
import "../../controls"
import "../../controls-uit" as HifiControls


Rectangle {
    id: openVrConfiguration

    width: parent.width
    height: parent.height
    anchors.fill: parent

    property int leftMargin: 55

    HifiConstants { id: hifi }

    color: hifi.colors.baseGray

    RalewayBold {
        id: head

        text: "Head:"
        size: 15

        color: "white"

        anchors.left: parent.left
        anchors.leftMargin: leftMargin
    }

    Row {
        id: headConfig
        anchors.top: head.bottom
        anchors.topMargin: 5
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 20
        spacing: 10

        HifiControls.CheckBox {
            id: headBox
            width: 15
            height: 15
            boxRadius: 7
        }
            
        RalewayBold {
            size: 15
            text: "Vive HMD"
            color: hifi.colors.lightGrayText
        }
        
        HifiControls.CheckBox {
            id: headPuckBox
            width: 15
            height: 15
            boxRadius: 7
        }

        RalewayBold {
            size: 15
            text: "Tracker"
            color: hifi.colors.lightGrayText
        }
    }

    RalewayBold {
        id: hands
        
        text: "Hands:"
        size: 15
        
        color: "white"

        anchors.top: headConfig.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: leftMargin
    }

    Row {
        id: handConfig
        anchors.top: hands.bottom
        anchors.topMargin: 5
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 20
        spacing: 10
        
        HifiControls.CheckBox {
            id: handBox
            width: 15
            height: 15
            boxRadius: 7
        }
        
        RalewayBold {
            size: 15
            text: "Controllers"
            color: hifi.colors.lightGrayText
        }
        
        HifiControls.CheckBox {
            id: handPuckBox
            width: 15
            height: 15
            boxRadius: 7
        }
        
        RalewayBold {
            size: 15
            text: "Trackers"
            color: hifi.colors.lightGrayText
        }
    }

    RalewayBold {
        id: additional
        
        text: "Additional Trackers"
        size: 15
        
        color: hifi.colors.white

        anchors.top: handConfig.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: leftMargin
    }

    Row {
        id: feetConfig
        anchors.top: additional.bottom
        anchors.topMargin: 15
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 20
        spacing: 10
        
        HifiControls.CheckBox {
            id: feetBox
            width: 15
            height: 15
            boxRadius: 7
        }
        
        RalewayBold {
            size: 15
            text: "Feet"
            color: hifi.colors.lightGrayText
        }
    }

    Row {
        id: hipConfig
        anchors.top: feetConfig.bottom
        anchors.topMargin: 15
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 20
        spacing: 10
        
        HifiControls.CheckBox {
            id: hipBox
            width: 15
            height: 15
            boxRadius: 7
        }
        
        RalewayBold {
            size: 15
            text: "Hips"
            color: hifi.colors.lightGrayText
        }

        RalewayBold {
            size: 12
            text: "requires feet"
            color: hifi.colors.lightGray
        }
    }


     Row {
        id: chestConfig
        anchors.top: hipConfig.bottom
        anchors.topMargin: 15
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 20
        spacing: 10
        
        HifiControls.CheckBox {
            id: chestBox
            width: 15
            height: 15
            boxRadius: 7
        }
        
        RalewayBold {
            size: 15
            text: "Chest"
            color: hifi.colors.lightGrayText
        }

        RalewayBold {
            size: 12
            text: "requires hips"
            color: hifi.colors.lightGray
        }
     }


     Row {
        id: shoulderConfig
        anchors.top: chestConfig.bottom
        anchors.topMargin: 15
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 20
        spacing: 10
        
        HifiControls.CheckBox {
            id: shoulderBox
            width: 15
            height: 15
            boxRadius: 7
        }
        
        RalewayBold {
            size: 15
            text: "Shoulders"
            color: hifi.colors.lightGrayText
        }

        RalewayBold {
            size: 12
            text: "requires hips"
            color: hifi.colors.lightGray
        }
     }

    Separator {
        id: bottomSeperator
        width: parent.width
        anchors.top: shoulderConfig.bottom
        anchors.topMargin: 10
    }
}
