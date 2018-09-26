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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.0
import QtQuick.Layouts 1.1
import Hifi 1.0 as Hifi
import "../../../styles-uit" as HifiStylesUit
import "../../../controls-uit" as HifiControlsUit



Rectangle {
    id: root

    property string installedApps
    property string resourceAccessEventText
    property var nextResourceObjectId: 0
    property var startDate
    signal sendToScript(var message)

    HifiStylesUit.HifiConstants { id: hifi }
    ListModel { id: resourceListModel }

    color: hifi.colors.white

    AnimatedImage {
        id: spinner;
        source: "spinner.gif"
        width: 74;
        height: width;
        anchors.verticalCenter: parent.verticalCenter;
        anchors.horizontalCenter: parent.horizontalCenter;
    }

    function fromScript(message) {
        switch (message.method) {
            case "newResourceObjectInTest":
                var resourceObject = message.resourceObject;
                resourceListModel.append(resourceObject);
                spinner.visible = false;
                break;
            case "nextObjectIdInTest":
                print("!!!! message from script! " + JSON.stringify(message));
                nextResourceObjectId = message.id;
                spinner.visible = false;
                break;
            case "resourceRequestEvent":
                try {
                    var date = new Date(JSON.parse(message.data.date));
                } catch(err) {
                    print("!!!!! Date conversion failed: " + JSON.stringify(message.data));
                }
                // XXX Eventually this date check goes away b/c we will
                // be able to match up resouce access events to resource
                // object ids, ignoring those that have -1 resource
                // object ids.
                if (date >= startDate) {
                    resourceAccessEventText += (
                        message.data.callerId + " " + message.data.extra +
                        " " + message.data.url +
                        " [" + date.toISOString() + "]\n"
                    );
                }
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
        return { "resourceObjectId": nextResourceObjectId++,
                 "resource": resource,
                 "assetType": assetType };
    }

    function installResourceObj(resourceObj) {
        if ("application" === resourceObj.assetType) {
            Commerce.installApp(resourceObj.resource);
        }
    }

    function addAllInstalledAppsToList() {
        var i, apps = Commerce.getInstalledApps().split(","), len = apps.length;
        for(i = 0; i < len - 1; ++i) {
            if (i in apps) {
                resourceListModel.append(buildResourceObj(apps[i]));
            }
        }
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

    Component.onCompleted: startDate = new Date()

    ColumnLayout {
        id: rootColumn
        spacing: 30

        HifiStylesUit.RalewayRegular {
            id: rootHeader
            text: "Marketplace Item Tester"
            height: 40
            width: paintedWidth
            size: 22
            color: hifi.colors.black
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.left: parent.left
            anchors.leftMargin: 12
        }

        Rectangle {
            height: root.height - 100
            width: root.width
            anchors.left: parent.left

            ScrollView {
                id: scrollView
                anchors.fill: parent
                anchors.rightMargin: 12
                anchors.bottom: parent.top
                anchors.bottomMargin: 20
                anchors.leftMargin: 12
                verticalScrollBarPolicy: Qt.ScrollBarAlwaysOn

                frameVisible: false

                contentItem: ListView {
                    spacing: 20
                    height: 200
                    model: resourceListModel
                    interactive: false

                    delegate: Column {
                        spacing: 8

                        RowLayout {
                            id: listRow
                            width: scrollView.width - 20
                            anchors.rightMargin: scrollView.rightMargin
                            spacing: 5

                            property var actions: {
                                "forward": function(resource, assetType, resourceObjectId){
                                    switch(assetType) {
                                        case "application":
                                            Commerce.openApp(resource);
                                            break;
                                        case "avatar":
                                            MyAvatar.useFullAvatarURL(resource);
                                            break;
                                        case "content set":
                                            urlHandler.handleUrl("hifi://localhost/0,0,0");
                                            Commerce.replaceContentSet(toUrl(resource), "");
                                            break;
                                        case "entity":
                                        case "wearable":
                                            rezEntity(resource, assetType, resourceObjectId);
                                            break;
                                        default:
                                            print("Marketplace item tester unsupported assetType " + assetType);
                                    }
                                },
                                "trash": function(resource, assetType){
                                    if ("application" === assetType) {
                                        Commerce.uninstallApp(resource);
                                    }
                                    sendToScript({
                                        method: "tester_deleteResourceObject",
                                        objectId: resourceListModel.get(index).id});
                                    resourceListModel.remove(index);
                                }
                            }

                            Column {
                                Layout.preferredWidth: scrollView.width * .6
                                spacing: 5
                                Text {
                                    width: listRow.width * .6
                                    text: {
                                        var match = resource.match(/\/([^/]*)$/);
                                        return match ? match[1] : resource;
                                    }
                                    font.pointSize: 12
                                    horizontalAlignment: Text.AlignBottom
                                    wrapMode: Text.WrapAnywhere
                                }
                                Text {
                                    width: listRow.width * .6
                                    text: resource
                                    font.pointSize: 8
                                    horizontalAlignment: Text.AlignBottom
                                    wrapMode: Text.WrapAnywhere
                                }
                            }

                            ComboBox {
                                id: comboBox

                                Layout.preferredWidth: listRow.width * .2

                                model: [
                                    "application",
                                    "avatar",
                                    "content set",
                                    "entity",
                                    "wearable",
                                    "unknown"
                                ]

                                currentIndex: (("entity or wearable" === assetType) ?
                                    model.indexOf("unknown") : model.indexOf(assetType))

                                Component.onCompleted: {
                                    onCurrentIndexChanged.connect(function() {
                                        assetType = model[currentIndex];
                                        sendToScript({
                                            method: "tester_updateResourceObjectAssetType",
                                            objectId: resourceListModel.get(index)["resourceObjectId"],
                                            assetType: assetType });
                                    });
                                }
                            }

                            Repeater {
                                model: [ "forward", "trash" ]

                                HifiStylesUit.HiFiGlyphs {
                                    property var glyphs: {
                                        "application": hifi.glyphs.install,
                                        "avatar": hifi.glyphs.avatar,
                                        "content set": hifi.glyphs.globe,
                                        "entity": hifi.glyphs.wand,
                                        "trash": hifi.glyphs.trash,
                                        "unknown": hifi.glyphs.circleSlash,
                                        "wearable": hifi.glyphs.hat,
                                    }
                                    text: (("trash" === modelData) ?
                                        glyphs.trash :
                                        glyphs[comboBox.model[comboBox.currentIndex]])
                                    size: ("trash" === modelData) ? 22 : 30
                                    color: hifi.colors.black
                                    horizontalAlignment: Text.AlignHCenter
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            listRow.actions[modelData](resource, comboBox.currentText, resourceObjectId);
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            id: detailsContainer

                            width: scrollView.width - 20
                            height: resourceDetails.isOpen ? 300 : 20
                            anchors.left: parent.left

                            HifiStylesUit.HiFiGlyphs {
                                id: detailsToggle
                                anchors.top: parent.top
                                text: resourceDetails.isOpen ? hifi.glyphs.minimize : hifi.glyphs.maximize
                                color: hifi.colors.black
                                size: 22
                                verticalAlignment: Text.AlignBottom
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: resourceDetails.isOpen = !resourceDetails.isOpen
                                }
                            }

                            TextArea {
                                id: resourceDetails

                                property var isOpen: false

                                width: detailsContainer.width - 20
                                height: detailsContainer.height
                                anchors.top: parent.top
                                anchors.left: detailsToggle.left
                                anchors.leftMargin: 20
                                verticalScrollBarPolicy: isOpen ? Qt.ScrollBarAsNeeded : Qt.ScrollBarAlwaysOff
                                frameVisible: isOpen
                                readOnly: true

                                text: {
                                    if (isOpen) {
                                        return resourceAccessEventText
                                    } else {
                                        return (resourceAccessEventText.split("\n").length - 1).toString() + " resources loaded..."
                                    }
                                }
                                font: Qt.font({ family: "Courier", pointSize: 8, weight: Font.Normal })
                                wrapMode: TextEdit.NoWrap
                            }
                        }

                        Rectangle {
                            width: listRow.width
                            height: 1
                            color: hifi.colors.black
                        }
                    }
                }
            }
        }

        Row {
            id: rootActions
            spacing: 20

            anchors.left: parent.left
            anchors.leftMargin: root.width / 6 - 10
            anchors.bottomMargin: 40
            anchors.bottom: parent.bottom

            property string currentAction
            property var actions: {
                "Load File": function(){
                    rootActions.currentAction = "load file";
                    Window.browseChanged.connect(onResourceSelected);
                    Window.browseAsync("Please select a file (*.app.json *.json *.fst *.json.gz)", "", "Assets (*.app.json *.json *.fst *.json.gz)");
                },
                "Load URL": function(){
                    rootActions.currentAction = "load url";
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
                    print("!!!! installing resource object");
                    installResourceObj(resourceObj);
                    print("!!!! notifying script of resource object");
                    sendToScript({
                        method: 'tester_newResourceObject',
                        resourceObject: resourceObj });
                }
            }

            Repeater {
                model: [ "Load File", "Load URL" ]
                HifiControlsUit.Button {
                    color: hifi.buttons.blue
                    fontSize: 20
                    text: modelData
                    width: root.width / 3
                    height: 40
                    onClicked: rootActions.actions[text]()
                }
            }
        }
    }
}
