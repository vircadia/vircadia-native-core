//
//  WindowHeader.qml
//  interface/resources/qml/android
//
//  Created by Gabriel Calero & Cristian Duarte on 23 Oct 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0
import "."
import "../styles" as HifiStyles
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../controls" as HifiControls
import ".."


// header
Rectangle {
    id: header

    // properties
    property string iconSource: ""
    property string titleText: ""
    property var extraItemInCenter: Item {}

    HifiStyles.HifiConstants { id: hifiStylesConstants }

    /*property var mockRectangle: Rectangle {
        anchors.fill: parent
        color: "#44FFFF00"
    }*/
    color: "#00000000"
    //color: "#55FF0000"
    width: parent.width
    height: android.dimen.headerHeight
    anchors.top : parent.top

    Image {
        id: windowIcon
        source: iconSource
        x: android.dimen.headerIconPosX
        y: android.dimen.headerIconPosY
        width: android.dimen.headerIconWidth
        height: android.dimen.headerIconHeight
    }

    /*HifiStylesUit.*/FiraSansSemiBold {
        id: windowTitle
        x: windowIcon.x + android.dimen.headerIconTitleDistance
        anchors.verticalCenter: windowIcon.verticalCenter
        text: titleText
        color: "#FFFFFF"
        font.letterSpacing: 2
        font.pixelSize: hifiStylesConstants.fonts.headerPixelSize * 2.15
    }
    Item {
    	height: 60
        anchors {
            left: windowTitle.right
            right: hideButton.left
            verticalCenter: windowIcon.verticalCenter
        }
        children: [ extraItemInCenter/*, mockRectangle */]
    }

    Rectangle {
        id: hideButton
        height: android.dimen.headerHideWidth
        width: android.dimen.headerHideHeight
        color: "#00000000"
        //color: "#CC00FF00"
        anchors {
            top: parent.top
            right: parent.right
            rightMargin: android.dimen.headerHideRightMargin
            topMargin: android.dimen.headerHideTopMargin
        }
        Image {
            id: hideIcon
            source: "../../../icons/hide.svg"
            width: android.dimen.headerHideIconWidth
            height: android.dimen.headerHideIconHeight
            anchors {
                horizontalCenter: parent.horizontalCenter
            }
        }
        /*HifiStyles.*/FiraSansRegular {
            anchors {
                top: hideIcon.bottom
                horizontalCenter: hideIcon.horizontalCenter
                topMargin: android.dimen.headerHideTextTopMargin
            }
            text: "HIDE"
            color: "#FFFFFF"
            font.pixelSize: hifiStylesConstants.fonts.pixelSize * 2.15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                hide();
            }
        }
    }
}