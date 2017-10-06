//
//  skyboxchanger.qml
//
//
//  Created by Cain Kilgore on 9th August 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick.Layouts 1.3

Rectangle {
    id: root;

    color: hifi.colors.baseGray;

    Item {
        id: titleBarContainer;
        // Size
        width: parent.width;
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        RalewaySemiBold {
            id: titleBarText;
            text: "Skybox Changer";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.fill: parent;
            anchors.leftMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }
        RalewaySemiBold {
            id: titleBarDesc;
            text: "Click an image to choose a new Skybox.";
            wrapMode: Text.Wrap
            // Text size
            size: 14;
            // Anchors
            anchors.fill: parent;
            anchors.top: titleBarText.bottom
            anchors.leftMargin: 16;
            anchors.rightMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }        
    }
    
    // This RowLayout could be a GridLayout instead for further expandability.
    // As this SkyboxChanger task only required 6 images, implementing GridLayout wasn't necessary.
    // In the future if this is to be expanded to add more Skyboxes, it might be worth changing this.
    RowLayout {
        id: row1
        anchors.top: titleBarContainer.bottom
        anchors.left: parent.left
        anchors.leftMargin: 30
        Layout.fillWidth: true
        anchors.topMargin: 30
        spacing: 10
        Image {
            width: 200; height: 200
            fillMode: Image.Stretch
            source: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_1.jpg"
            clip: true
            id: preview1
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    sendToScript({method: 'changeSkybox', url: 'http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/1.jpg'});
                }
            }
            Layout.fillWidth: true
        }
        Image {
            width: 200; height: 200
            fillMode: Image.Stretch
            source: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_2.jpg"
            clip: true
            id: preview2
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    sendToScript({method: 'changeSkybox', url: 'http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/2.png'});
                }
            }
        }
    }
    RowLayout {
        id: row2
        anchors.top: row1.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        Layout.fillWidth: true
        anchors.leftMargin: 30
        spacing: 10
        Image {
            width: 200; height: 200
            fillMode: Image.Stretch
            source: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_3.jpg"
            clip: true
            id: preview3
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    sendToScript({method: 'changeSkybox', url: 'http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/3.jpg'});
                }
            }
        }
        Image {
            width: 200; height: 200
            fillMode: Image.Stretch
            source: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_4.jpg"
            clip: true
            id: preview4
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    sendToScript({method: 'changeSkybox', url: 'http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/4.jpg'});
                }
            }
        }
    }
    RowLayout {
        id: row3
        anchors.top: row2.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        Layout.fillWidth: true
        anchors.leftMargin: 30
        spacing: 10
        Image {
            width: 200; height: 200
            fillMode: Image.Stretch
            source: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_5.jpg"
            clip: true
            id: preview5
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    sendToScript({method: 'changeSkybox', url: 'http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/5.png'});
                }
            }
        }
        Image {
            width: 200; height: 200
            fillMode: Image.Stretch
            source: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/thumbnails/thumb_6.jpg"
            clip: true
            id: preview6
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    sendToScript({method: 'changeSkybox', url: 'http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxes/6.jpg'});
                }
            }
        }
    }
    
    signal sendToScript(var message);
    
}
