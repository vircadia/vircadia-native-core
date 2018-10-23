//
//  ModalFrame.qml
//
//  Created by Bradley Austin Davis on 15 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "."
import "../controls-uit"
import "../styles-uit"


Rectangle {
    HifiConstants { id: hifi }

    id: frameContent

    readonly property bool hasTitle: root.title != ""

    readonly property int frameMarginLeft: hifi.dimensions.modalDialogMargin.x
    readonly property int frameMarginRight: hifi.dimensions.modalDialogMargin.x
    readonly property int frameMarginTop: hifi.dimensions.modalDialogMargin.y + (frameContent.hasTitle ? hifi.dimensions.modalDialogTitleHeight + 10 : 0)
    readonly property int frameMarginBottom: hifi.dimensions.modalDialogMargin.y

    border {
        width: hifi.dimensions.borderWidth
        color: hifi.colors.lightGrayText80
    }

    radius: hifi.dimensions.borderRadius
    color: hifi.colors.faintGray
    Item {
        id: frameTitle
        visible: frameContent.hasTitle
        
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
            fill: parent
            topMargin: frameMarginTop
            leftMargin: frameMarginLeft
            rightMargin: frameMarginRight
            //bottomMargin: frameMarginBottom
        }
        
        Item {
            width: title.width + (icon.text !== "" ? icon.width + hifi.dimensions.contentSpacing.x : 20)
            
            onWidthChanged: root.titleWidth = width
            
            HiFiGlyphs {
                id: icon
                text: root.iconText ? root.iconText : ""
                size: root.iconSize ? root.iconSize : 30
                color: hifi.colors.lightGray
                visible: true
                anchors.verticalCenter: title.verticalCenter
                anchors.leftMargin: 50
                anchors.left: parent.left
            }
            
            RalewayRegular {
                id: title
                text: root.title
                elide: Text.ElideRight
                color: hifi.colors.baseGrayHighlight
                size: hifi.fontSizes.overlayTitle
                y: -hifi.dimensions.modalDialogTitleHeight
                anchors.rightMargin: -50
                anchors.right: parent.right
                //anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: hifi.colors.lightGray
        }
        
    }
    
}
