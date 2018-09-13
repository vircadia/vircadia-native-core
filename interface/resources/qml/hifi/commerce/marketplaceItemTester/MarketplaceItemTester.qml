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
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.0
import QtQuick.Layouts 1.1
import Hifi 1.0 as Hifi
import "../../../styles-uit" as HifiStylesUit
import "../../../controls-uit" as HifiControlsUit



Rectangle {
    id:root
    HifiStylesUit.HifiConstants { id: hifi }
    color: hifi.colors.white
    ListModel { id: listModel }
    ListView {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.bottomMargin: 40
        model: listModel
        spacing: 5
        delegate: RowLayout {
            anchors.left: parent.left
            width: parent.width
            spacing: 5
            Text {
                text: {
                    var match = resource.match(/\/([^/]*)$/);
                    return match ? match[1] : resource;
                }
                font.pointSize: 12
                Layout.preferredWidth: root.width * .6
                horizontalAlignment: Text.AlignBottom
            }
            Text {
                text: assetType
                font.pointSize: 10
                Layout.preferredWidth: root.width * .2
                horizontalAlignment: Text.AlignBottom
            }
            property var actions: {
                "forward": function(resource, assetType){
                    if ("application" == assetType) {
                        Commerce.installApp(resource);
                        Commerce.openApp(resource);
                    }
                    // XXX support other resource types here.
                },
                "trash": function(){
                    if ("application" == assetType) {
                        Commerce.uninstallApp(resource);
                    }
                    // XXX support other resource types here.
                    listModel.remove(index);
                }
            }
            Repeater {
                model: [
                    { "name": "forward", "glyph": hifi.glyphs.forward, "size": 30 },
                    { "name": "trash", "glyph": hifi.glyphs.trash, "size": 22}
                ]
                HifiStylesUit.HiFiGlyphs {
                    text: modelData.glyph
                    size: modelData.size
                    color: hifi.colors.black
                    horizontalAlignment: Text.AlignHCenter
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            actions[modelData.name](resource, assetType);
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
            function assetType(resource) {
                return (resource.match(/\.app\.json$/) ? "application" :
                        resource.match(/\.(?:fbx|fst)$/) ? "avatar" :
                        resource.match(/\.json\.gz$/) ? "content set" :
                        resource.match(/\.json$/) ? "entity or wearable" :
                        "unknown")
            }
            function onResourceSelected(resource) {
                // It is possible that we received the present signal
                // from something other than our browserAsync window.
                // Alas, there is nothing we can do about that so charge
                // ahead as though we are sure the present signal is one
                // we expect.
                if ("load file" == currentAction) {
                    print("disconnecting load file");
                    Window.browseChanged.disconnect(onResourceSelected);
                } else if ("load url" == currentAction) {
                    print("disconnecting load url");
                    Window.promptTextChanged.disconnect(onResourceSelected);
                }
                if (resource) {
                    listModel.append( {
                        "resource": resource.trim(),
                        "assetType": assetType(resource.trim()) } );
                }
            }
            property var actions: {
                "Load File": function(){
                    rootActions.currentAction = "load file";
                    Window.browseChanged.connect(onResourceSelected);
                    Window.browseAsync("Please select a file", "", "Assets (*.app.json *.json *.fbx *.json.gz)");
                },
                "Load URL": function(){
                    rootActions.currentAction = "load url";
                    Window.promptTextChanged.connect(onResourceSelected);
                    Window.promptAsync("Please enter a URL", "");
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
