//
//  marketplaceItemTester
//  qml/hifi/commerce/marketplaceItemTester
//
//  Load items not in the marketplace for testing purposes
//
//  Created by Zach Fox on 2018-09-05
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
    property var nextResourceObjectId: 0
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
                nextResourceObjectId = message.id;
                spinner.visible = false;
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
        return { "id": nextResourceObjectId++,
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

    function rezEntity(resource, entityType) {
        sendToScript({
            method: 'tester_rezClicked',
            itemHref: toUrl(resource),
            itemType: entityType});
    }

    ListView {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.bottomMargin: 40
        anchors.rightMargin: 12
        model: resourceListModel
        spacing: 5
        interactive: false

        delegate: RowLayout {
            anchors.left: parent.left
            width: parent.width
            spacing: 5

            property var actions: {
                "forward": function(resource, assetType){
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
                            rezEntity(resource, assetType);
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
                Layout.preferredWidth: root.width * .6
                spacing: 5
                Text {
                    text: {
                        var match = resource.match(/\/([^/]*)$/);
                        return match ? match[1] : resource;
                    }
                    font.pointSize: 12
                    horizontalAlignment: Text.AlignBottom
                }
                Text {
                    text: resource
                    font.pointSize: 8
                    width: root.width * .6
                    horizontalAlignment: Text.AlignBottom
                    wrapMode: Text.WrapAnywhere
                }
            }

            ComboBox {
                id: comboBox

                Layout.preferredWidth: root.width * .2

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
                            objectId: resourceListModel.get(index)["id"],
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
                            actions[modelData](resource, comboBox.currentText);
                        }
                    }
                }
            }
        }

        headerPositioning: ListView.OverlayHeader
        header: HifiStylesUit.RalewayRegular {
            id: rootHeader
            text: "Marketplace Item Tester"
            height: 80
            width: paintedWidth
            size: 22
            color: hifi.colors.black
            anchors.left: parent.left
            anchors.leftMargin: 12
        }

        footerPositioning: ListView.OverlayFooter
        footer: Row {
            id: rootActions
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

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
                switch(currentAction) {
                    case "load file":
                        Window.browseChanged.disconnect(onResourceSelected);
                        break
                    case "load url":
                        Window.promptTextChanged.disconnect(onResourceSelected);
                        break;
                }
                if (resource) {
                    var resourceObj = buildResourceObj(resource);
                    installResourceObj(resourceObj);
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
                    onClicked: actions[text]()
                }
            }
        }
    }
}
