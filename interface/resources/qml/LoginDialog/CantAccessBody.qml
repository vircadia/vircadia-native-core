//
//  CantAccessBody.qml
//
//  Created by Wayne Chen on 11/28/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit

Item {
    id: cantAccessBody
    clip: true
    width: root.width
    height: root.height
    property int textFieldHeight: 31
    property string fontFamily: "Raleway"
    property int fontSize: 15
    // property int textFieldFontSize: !root.isTablet ? !root.isOverlay : hifi.fontSizes.textFieldInput ? hifi.fontSizes.textFieldInput : 18
    property int textFontSize: 24
    property bool fontBold: true

    QtObject {
        id: d
        readonly property int minWidth: 480
        readonly property int minWidthButton: !root.isTablet ? 256 : 174
        property int maxWidth: root.isTablet ? 1280 : root.width
        readonly property int minHeight: 120
        // readonly property int minHeightButton: !root.isTablet ? 56 : 42
        readonly property int minHeightButton: 36
        property int maxHeight: root.isTablet ? 720 : root.height

        function resize() {
            maxWidth = root.isTablet ? 1280 : root.width;
            maxHeight = root.isTablet ? 720 : root.height;
            var targetWidth = Math.max(titleWidth, mainContainer.width);
            var targetHeight =  hifi.dimensions.contentSpacing.y + mainContainer.height + 4 * hifi.dimensions.contentSpacing.y;

            var newWidth = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            if (!isNaN(newWidth)) {
                parent.width = root.width = newWidth;
            }

            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight)) + hifi.dimensions.contentSpacing.y;
        }
    }

    Item {
        id: mainContainer
        anchors.fill: parent
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Item {
            id: textContainer
            width: parent.width
            height: 0.45 * parent.height
            anchors {
                top: parent.top
                topMargin: bannerContainer.height + 0.25 * parent.height
                left: parent.left
            }
            TextMetrics {
                id: titleTextMetrics
                font: titleText.font
                text: titleText.text
            }
            Text {
                id: titleText
                anchors {
                    top: parent.top
                    topMargin: 0.2 * parent.height
                    left: parent.left
                    leftMargin: (parent.width - titleTextMetrics.width) / 2
                }
                text: qsTr("Can't Access Account")
                font.pixelSize: cantAccessBody.textFontSize + 10
                font.bold: cantAccessBody.fontBold
                color: "white"
                lineHeight: 2
                lineHeightMode: Text.ProportionalHeight
                horizontalAlignment: Text.AlignHCenter
            }

            TextMetrics {
                id: bodyTextMetrics
                font: bodyText.font
                text: bodyText.text
            }
            Text {
                id: bodyText
                anchors {
                    top: titleText.bottom
                    topMargin: hifi.dimensions.contentSpacing.y
                    left: parent.left
                }
                text: qsTr("Please navigate to your default browser to recover your account.\nIf you are in a VR headset, please take it off.")
                font.pixelSize: cantAccessBody.textFontSize
                color: "white"
                wrapMode: Text.WordWrap
                lineHeight: 1
                lineHeightMode: Text.ProportionalHeight
                horizontalAlignment: Text.AlignHCenter
                Component.onCompleted: {
                    bodyText.text = root.isTablet ? qsTr("Please navigate to your default browser\nto recover your account.\nIf you are in a VR headset, please take it off.") :
                        qsTr("Please navigate to your default browser to recover your account.\nIf you are in a VR headset,\nplease take it off.");
                    bodyTextMetrics.text = bodyText.text;
                    bodyText
                    bodyText.anchors.leftMargin = (parent.width - bodyTextMetrics.width) / 2;

                }
            }
        }

        HifiControlsUit.Button {
            id: okButton
            height: d.minHeightButton
            anchors {
                bottom: parent.bottom
                right: parent.right
                margins: 3 * hifi.dimensions.contentSpacing.y
            }
            text: qsTr("OK")
            fontFamily: cantAccessBody.fontFamily
            fontSize: cantAccessBody.fontSize
            fontBold: cantAccessBody.fontBold
            onClicked: {
                bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": ""});
            }
        }

    }
    Component.onCompleted: {
        d.resize();
    }
}
