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
import "../styles-uit"
import "../controls-uit" as HifiControls

Item {
    id: root;
    HifiConstants { id: hifi; }
    
    property var defaultThumbnails: [];
    property var defaultFulls: [];
    
    SkyboxSelectionModel {
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
        for (var i = 0; i < arr.length; i++) {
            defaultThumbnails.push(arr[i].thumb);
            defaultFulls.push(arr[i].full);
        }
        setDefaultSkyboxes();
    }
    
    function setDefaultSkyboxes() {
        for (var i = 0; i < skyboxModel.count; i++) {
            skyboxModel.setProperty(i, "thumbnailPath", defaultThumbnails[i]);
            skyboxModel.setProperty(i, "fullSkyboxPath", defaultFulls[i]);
        }
    }
    
    function shuffle(array) {
        var tmp, current, top = array.length;
        if (top) {
            while (--top) {
                current = Math.floor(Math.random() * (top + 1));
                tmp = array[current];
                array[current] = array[top];
                array[top] = tmp;
            }
        }
      return array;
    }

    function chooseRandom() {
        for (var a = [], i=0; i < defaultFulls.length; ++i) {
            a[i] = i;
        }
        
        a = shuffle(a);
        
        for (var i = 0; i < skyboxModel.count; i++) {
            skyboxModel.setProperty(i, "thumbnailPath", defaultThumbnails[a[i]]);
            skyboxModel.setProperty(i, "fullSkyboxPath", defaultFulls[a[i]]);
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
        HifiControls.Button {
            id: randomButton
            text: "Randomize"
            color: hifi.buttons.blue
            colorScheme: root.colorScheme
            width: 100
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 5
            anchors.rightMargin: 40
            onClicked: {
                chooseRandom()
            }
        }
    }
    
    GridView {
        id: gridView
        interactive: false
        clip: true
        anchors.top: titleBarContainer.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        width: 400
        height: parent.height
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
    }
    
    signal sendToScript(var message);
}