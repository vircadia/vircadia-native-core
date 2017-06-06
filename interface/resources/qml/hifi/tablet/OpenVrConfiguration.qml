//
//  Created by Dante Ruiz on 6/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import "../../styles-uit"
import "../../controls"
import "../../controls-uit" as HifiControls


Rectangle {
    id: openVrConfiguration

    width: parent.width
    height: parent.height
    anchors.fill: parent

    property int leftMargin: 75
    property string pluginName: ""

    HifiConstants { id: hifi }

    color: hifi.colors.baseGray

    RalewayBold {
        id: head

        text: "Head:"
        size: 12

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
            size: 12
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
            size: 12
            text: "Tracker"
            color: hifi.colors.lightGrayText
        }
    }

    RalewayBold {
        id: hands
        
        text: "Hands:"
        size: 12
        
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
            size: 12
            text: "Controllers"
            color: hifi.colors.lightGrayText
        }
        
        HifiControls.CheckBox {
            id: handPuckBox
            width: 12
            height: 15
            boxRadius: 7
        }
        
        RalewayBold {
            size: 12
            text: "Trackers"
            color: hifi.colors.lightGrayText
        }
    }

    RalewayBold {
        id: additional
        
        text: "Additional Trackers"
        size: 12
        
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
            size: 12
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
            size: 12
            text: "Hips"
            color: hifi.colors.lightGrayText
        }

        RalewayRegular {
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
            size: 12
            text: "Chest"
            color: hifi.colors.lightGrayText
        }

        RalewayRegular {
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
            size: 12
            text: "Shoulders"
            color: hifi.colors.lightGrayText
        }

        RalewayRegular {
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


    Rectangle {
        id: calibrationButton
        width: 200
        height: 35
        radius: 6

        color: hifi.colors.blueHighlight

        anchors.top: bottomSeperator.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: leftMargin


        HiFiGlyphs {
            id: calibrationGlyph
            text: hifi.glyphs.sliders
            size: 36
            color: hifi.colors.white

            anchors.horizontalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 30
            
        }
        
        RalewayRegular {
            id: calibrate
            text: "CALIBRATE"
            size: 17
            color: hifi.colors.white

            anchors.left: calibrationGlyph.right
            anchors.top: parent.top
            anchors.topMargin: 8
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                InputCalibration.calibratePlugin(pluginName);
            }
        }
    }


    
}
