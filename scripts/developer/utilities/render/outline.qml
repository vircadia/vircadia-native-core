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
import QtQuick 2.5
import QtQuick.Controls 1.4
import "outlinePage"

Item {
    id: root
    property var debugConfig: Render.getConfig("RenderMainView.OutlineDebug")
    signal sendToScript(var message);

    Column {
        spacing: 8
        anchors.fill: parent

        CheckBox {
            id: debug
            text: "View Mask"
            checked: root.debugConfig["viewMask"]
            onCheckedChanged: {
                root.debugConfig["viewMask"] = checked;
            }
        }

        TabView {
            id: tabs
            width: 384
            height: 400

            onCurrentIndexChanged: {
                sendToScript(currentIndex)
            }

            Tab {
                title: "Outl.0"
                OutlinePage {
                    outlineIndex: 0
                }
            }
            Tab {
                title: "Outl.1"
                OutlinePage {
                    outlineIndex: 1
                }
            }
            Tab {
                title: "Outl.2"
                OutlinePage {
                    outlineIndex: 2
                }
            }
            Tab {
                title: "Outl.3"
                OutlinePage {
                    outlineIndex: 3
                }
            }
        }
    }
}
