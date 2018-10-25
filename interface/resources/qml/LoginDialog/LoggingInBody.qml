//
//  LoggingInBody.qml
//
//  Created by Wayne Chen on 10/18/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
Item {

    Item {
        id: mainContainer
        width: root.pane.width
        height: root.pane.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Rectangle {
            id: opaqueRect
            height: parent.height
            width: parent.width
            opacity: 0.9
            color: "black"
        }

        Item {
            id: bannerContainer
            width: parent.width
            height: banner.height
            anchors {
                top: parent.top
                topMargin: 85
            }
            Image {
                id: banner
                anchors.centerIn: parent
                source: "../../images/high-fidelity-banner.svg"
                horizontalAlignment: Image.AlignHCenter
            }
        }

        Item {
            id: loggingInContainer
            width: parent.width
            height: parent.height
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            visible: false

            Item {
                id: loggingInHeader
                width: parent.width
                height: 0.5 * parent.height
                anchors {
                    top: parent.top
                }
                TextMetrics {
                    id: loggingInGlyphTextMetrics;
                    font: loggingInGlyph.font;
                    text: loggingInGlyph.text;
                }
                HifiStylesUit.HiFiGlyphs {
                    id: loggingInGlyph;
                    text: hifi.glyphs.steamSquare;
                    // Color
                    color: "white";
                    // Size
                    size: 31;
                    // Anchors
                    anchors.right: loggingInText.left;
                    anchors.rightMargin: signInBody.loggingInGlyphRightMargin
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: hifi.dimensions.contentSpacing.y
                    // Alignment
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    visible: loginDialog.isSteamRunning();
                }

                TextMetrics {
                    id: loggingInTextMetrics;
                    font: loggingInText.font;
                    text: loggingInText.text;
                }
                Text {
                    id: loggingInText;
                    width: loggingInTextMetrics.width
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: hifi.dimensions.contentSpacing.y
                    anchors.left: parent.left;
                    anchors.leftMargin: (parent.width - loggingInTextMetrics.width) / 2
                    color: "white";
                    font.family: signInBody.fontFamily
                    font.pixelSize: signInBody.fontSize
                    font.bold: signInBody.fontBold
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: "Logging in"
                }
            }
            Item {
                id: loggingInFooter
                width: parent.width
                height: 0.5 * parent.height
                anchors {
                    top: loggingInHeader.bottom
                }
                AnimatedImage {
                    id: linkAccountSpinner
                    source: "../../icons/loader-snake-64-w.gif"
                    width: 128
                    height: width
                    anchors.left: parent.left;
                    anchors.leftMargin: (parent.width - width) / 2;
                    anchors.top: parent.top
                    anchors.topMargin: hifi.dimensions.contentSpacing.y
                }
                TextMetrics {
                    id: loggedInGlyphTextMetrics;
                    font: loggedInGlyph.font;
                    text: loggedInGlyph.text;
                }
                HifiStylesUit.HiFiGlyphs {
                    id: loggedInGlyph;
                    text: hifi.glyphs.steamSquare;
                    // color
                    color: "white"
                    // Size
                    size: 78;
                    // Anchors
                    anchors.left: parent.left;
                    anchors.leftMargin: (parent.width - loggedInGlyph.size) / 2;
                    anchors.top: parent.top
                    anchors.topMargin: hifi.dimensions.contentSpacing.y
                    // Alignment
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    visible: loginDialog.isSteamRunning();

                }
            }
        }
    }
}
