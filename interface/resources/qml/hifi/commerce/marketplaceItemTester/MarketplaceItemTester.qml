//
//  marketplaceItemTester
//  qml/hifi/commerce/marketplaceItemTester
//
//  Load items not in the marketplace for testing purposes
//
//  Created by Kerry Ivan Kurian on 2018-09-05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3
import Hifi 1.0 as Hifi
import stylesUit 1.0 as HifiStylesUit
import controlsUit 1.0 as HifiControlsUit




Rectangle {
    id: root

    property string installedApps
    property var nextResourceObjectId: 0

    HifiStylesUit.HifiConstants { id: hifi }
    ListModel { id: resourceListModel }

    color: hifi.colors.darkGray

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer
        // Size
        width: root.width
        height: 50
        // Anchors
        anchors.left: parent.left
        anchors.top: parent.top

        // Title bar text
        HifiStylesUit.RalewaySemiBold {
            id: titleBarText
            text: "Marketplace Item Tester"
            // Text size
            size: 24
            // Anchors
            anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.left: parent.left
            anchors.leftMargin: 16
			width: paintedWidth
            // Style
            color: hifi.colors.lightGrayText
            // Alignment
            horizontalAlignment: Text.AlignHLeft
            verticalAlignment: Text.AlignVCenter
        }

        // Separator
        HifiControlsUit.Separator {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 1
        }
    }
    //
    // TITLE BAR END
    //

    Rectangle {
        id: spinner
        z: 999
        anchors.top: titleBarContainer.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttonContainer.top
        color: hifi.colors.darkGray

        AnimatedImage {
            source: "spinner.gif"
            width: 74
            height: width
            anchors.centerIn: parent
        }
    }

    Rectangle {
        id: instructionsContainer
        z: 998
        color: hifi.colors.darkGray
        visible: resourceListModel.count === 0 && !spinner.visible
        anchors.top: titleBarContainer.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.bottom: buttonContainer.top
        anchors.bottomMargin: 20

        HifiStylesUit.RalewayRegular {
            text: "Use Marketplace Item Tester to test out your items before submitting them to the Marketplace." +
                "\n\nUse one of the buttons below to load your item."
            // Text size
            size: 20
            // Anchors
            anchors.fill: parent
            // Style
            color: hifi.colors.lightGrayText
            wrapMode: Text.Wrap
            // Alignment
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    ListView {
        id: itemList
        visible: !instructionsContainer.visible
        anchors.top: titleBarContainer.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttonContainer.top
        anchors.bottomMargin: 20
        ScrollBar.vertical: ScrollBar {
            visible: !instructionsContainer.visible
            policy: ScrollBar.AlwaysOn
            parent: itemList.parent
            anchors.top: itemList.top
            anchors.right: itemList.right
            anchors.bottom: itemList.bottom
            width: 16
        }
        clip: true
        model: resourceListModel
        spacing: 8

        delegate: ItemUnderTest { }
    }

    Item {
        id: buttonContainer

        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        height: 40

        property string currentAction
        property var actions: {
            "Load File": function() {
                buttonContainer.currentAction = "load file";
                Window.browseChanged.connect(onResourceSelected);
                Window.browseAsync("Please select a file (*.app.json *.json *.fst *.json.gz)", "", "Assets (*.app.json *.json *.fst *.json.gz)");
            },
            "Load URL": function() {
                buttonContainer.currentAction = "load url";
                Window.promptTextChanged.connect(onResourceSelected);
                Window.promptAsync("Please enter a URL", "");
            }
        }

        function onResourceSelected(resource) {
            // It is possible that we received the present signal
            // from something other than our browserAsync window.
            // Alas, there is nothing we can do about that so charge
            // ahead as though we are sure the present signal is one
            // we expect.
            print("!!!! resource selected");
            switch(currentAction) {
                case "load file":
                    Window.browseChanged.disconnect(onResourceSelected);
                    break
                case "load url":
                    Window.promptTextChanged.disconnect(onResourceSelected);
                    break;
            }
            if (resource) {
                print("!!!! building resource object");
                var resourceObj = buildResourceObj(resource);
                print("!!!! notifying script of resource object");
                sendToScript({
                    method: 'tester_newResourceObject',
                    resourceObject: resourceObj
                });
            }
        }

        HifiControlsUit.Button {
            enabled: !spinner.visible
            anchors.right: parent.horizontalCenter
            anchors.rightMargin: width/4
            anchors.verticalCenter: parent.verticalCenter
            color: hifi.buttons.blue
            fontSize: 20
            text: "Load File"
            width: parent.width / 3
            height: parent.height
            onClicked: buttonContainer.actions[text]()
        }

        HifiControlsUit.Button {
            enabled: !spinner.visible
            anchors.left: parent.horizontalCenter
            anchors.leftMargin: width/4
            anchors.verticalCenter: parent.verticalCenter
            color: hifi.buttons.blue
            fontSize: 20
            text: "Load URL"
            width: parent.width / 3
            height: parent.height
            onClicked: buttonContainer.actions[text]()
        }
    }

    function fromScript(message) {
        switch (message.method) {
            case "newResourceObjectInTest":
                var resourceObject = message.resourceObject;
                resourceListModel.clear(); // REMOVE THIS once we support specific referrers
                resourceListModel.append(resourceObject);
                spinner.visible = false;
                break;
            case "nextObjectIdInTest":
                print("!!!! message from script! " + JSON.stringify(message));
                nextResourceObjectId = message.id;
                spinner.visible = false;
                break;
            case "resourceRequestEvent":
                // When we support multiple items under test simultaneously,
                // we'll have to replace "0" with the correct index.
                resourceListModel.setProperty(0, "resourceAccessEventText", message.resourceAccessEventText);
                break;
        }
    }

    function buildResourceObj(resource) {
        resource = resource.trim();
        var assetType = (resource.match(/\.app\.json$/) ? "application" :
                         resource.match(/\.fst$/) ? "avatar" :
                         resource.match(/\.json\.gz$/) ? "content set" :
                         resource.match(/\.json$/) ? "entity or wearable" :
                         "unknown");
        // Uncomment this once we support more than one item in test at the same time
        //nextResourceObjectId++;
        return { "resourceObjectId": nextResourceObjectId,
                 "resource": resource,
                 "assetType": assetType };
    }

    function toUrl(resource) {
        var httpPattern = /^http/i;
        return httpPattern.test(resource) ? resource : "file:///" + resource;
    }

    function rezEntity(resource, entityType, resourceObjectId) {
        print("!!!! tester_rezClicked");
        sendToScript({
            method: 'tester_rezClicked',
            itemHref: toUrl(resource),
            itemType: entityType,
            itemId:   resourceObjectId });
    }

    signal sendToScript(var message)
}
