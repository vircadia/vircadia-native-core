//
//  outline.qml
//  developer/utilities/render
//
//  Olivier Prat, created on 08/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import "outlinePage"
import "qrc:///qml/styles-uit"
import "qrc:///qml/controls-uit" as HifiControls

Rectangle {
    id: root
    HifiConstants { id: hifi;}
    color: hifi.colors.baseGray;
    anchors.margins: hifi.dimensions.contentMargin.x

    property var debugConfig: Render.getConfig("RenderMainView.OutlineDebug")
    signal sendToScript(var message);

    Column {
        spacing: 5
        anchors.left: parent.left
        anchors.right: parent.right       
        anchors.margins: hifi.dimensions.contentMargin.x  

        Row {
            spacing: 10
            anchors.left: parent.left
            anchors.right: parent.right 

            HifiControls.CheckBox {
                id: debug
                text: "View Mask"
                checked: root.debugConfig["viewMask"]
                onCheckedChanged: {
                    root.debugConfig["viewMask"] = checked;
                }
            }
            HifiControls.CheckBox {
                text: "Hover select"
                checked: false
                onCheckedChanged: {
                    sendToScript("pick "+checked.toString())
                }
            }
            HifiControls.CheckBox {
                text: "Add to selection"
                checked: false
                onCheckedChanged: {
                    sendToScript("add "+checked.toString())
                }
            }
        }

        TabView {
            id: tabs
            width: 384
            height: 400

            onCurrentIndexChanged: {
                sendToScript("outline "+currentIndex)
            }

            Repeater {
                model: [ 0, 1, 2, 3 ]
                Tab {
                    title: "Outl."+modelData
                    OutlinePage {
                        outlineIndex: modelData
                    }
                }
            }
        }
    }
}
