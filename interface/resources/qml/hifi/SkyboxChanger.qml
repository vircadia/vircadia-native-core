//
//  SkyboxChanger.qml
//  qml/hifi
//
//  Created by Cain Kilgore on 9th August 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import stylesUit 1.0
import controlsUit 1.0 as HifiControls
import QtQuick.Controls 2.2

Item {
    id: root;
    HifiConstants { id: hifi; }
    
    property var defaultThumbnails: [];
    property var defaultFulls: [];
    
    ListModel {
        id: skyboxModel;
    }
    
    function getSkyboxes() {
        var xmlhttp = new XMLHttpRequest();
        var url = "http://mpassets.highfidelity.com/5fbdbeef-1cf8-4954-811d-3d4acbba4dc9-v1/skyboxes.json";
        xmlhttp.onreadystatechange = function() {
            if (xmlhttp.readyState === XMLHttpRequest.DONE && xmlhttp.status === 200) {
                sortSkyboxes(xmlhttp.responseText);
            }
        }
        xmlhttp.open("GET", url, true);
        xmlhttp.send();
    }
    
    function sortSkyboxes(response) {
        var arr = JSON.parse(response);
        var arrLength = arr.length;
        for (var i = 0; i < arrLength; i++) {
            defaultThumbnails.push(arr[i].thumb);
            defaultFulls.push(arr[i].full);
            skyboxModel.append({});
        }
        setSkyboxes();
    }
    
    function setSkyboxes() {
        for (var i = 0; i < skyboxModel.count; i++) {
            skyboxModel.setProperty(i, "thumbnailPath", defaultThumbnails[i]);
            skyboxModel.setProperty(i, "fullSkyboxPath", defaultFulls[i]);
        }
    }
    
    Component.onCompleted: {
        getSkyboxes();
    }
    
    Item {
        id: titleBarContainer;
        width: parent.width;
        height: childrenRect.height;
        anchors.left: parent.left;
        anchors.top: parent.top;
        anchors.right: parent.right;
        anchors.topMargin: 20;
        RalewayBold {
            id: titleBarText;
            text: "Skybox Changer";
            size: hifi.fontSizes.overlayTitle;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 40
            height: paintedHeight;
            color: hifi.colors.lightGrayText;
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }
        RalewaySemiBold {
            id: titleBarDesc;
            text: "Click an image to choose a new Skybox.";
            wrapMode: Text.Wrap
            size: 14;
            anchors.top: titleBarText.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 40
            height: paintedHeight;
            color: hifi.colors.lightGrayText;
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }
    }
    
    GridView {
        id: gridView
        interactive: true
        clip: true
        anchors.top: titleBarContainer.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        width: 400
        anchors.bottom: parent.bottom
        currentIndex: -1
        cellWidth: 200
        cellHeight: 200
        model: skyboxModel
        delegate: Item {
            width: gridView.cellWidth
            height: gridView.cellHeight
            Item {
                anchors.fill: parent
                Image {
                    source: thumbnailPath
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    fillMode: Image.Stretch
                    sourceSize.width: parent.width
                    sourceSize.height: parent.height
                    mipmap: true
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    sendToScript({method: 'changeSkybox', url: fullSkyboxPath});
                }
            }
        }
        ScrollBar.vertical: ScrollBar {
            parent: gridView.parent
            anchors.top: gridView.top
            anchors.left: gridView.right
            anchors.bottom: gridView.bottom
            anchors.leftMargin: 10
            width: 19
        }
    }
    
    signal sendToScript(var message);
}