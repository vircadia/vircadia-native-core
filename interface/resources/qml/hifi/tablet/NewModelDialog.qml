//
//  NewModelDialog.qml
//  qml/hifi
//
//  Created by Seth Alves on 2017-2-10
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import "../../styles-uit"
import "../../controls-uit"

Rectangle {
    id: newModelDialog
    // width: parent.width
    // height: parent.height
    HifiConstants { id: hifi }
    color: hifi.colors.baseGray;
    property var eventBridge;
    signal sendToScript(var message);

    Column {
        id: column1
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        anchors.bottomMargin: 10
        anchors.topMargin: 10
        anchors.fill: parent
        spacing: 5

        Text {
            id: text1
            text: qsTr("Model URL")
            color: "#ffffff"
            font.pixelSize: 12
        }

        TextInput {
            id: modelURL
            height: 20
            text: qsTr("")
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 0
            font.pixelSize: 12
        }

        Row {
            id: row1
            height: 400
            spacing: 30
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 0

            Column {
                id: column2
                width: 200
                height: 400
                spacing: 10

                CheckBox {
                    id: dynamic
                    text: qsTr("Dynamic")
                    
                }

                Row {
                    id: row2
                    width: 200
                    height: 400
                    spacing: 20

                    Image {
                        id: image1
                        width: 30
                        height: 30
                        source: "qrc:/qtquickplugin/images/template_image.png"
                    }

                    Text {
                        id: text2
                        width: 160
                        color: "#ffffff"
                        text: qsTr("Models with automatic collisions set to 'Exact' cannot be dynamic")
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                    }
                }
            }

            Column {
                id: column3
                height: 400
                spacing: 10

                Text {
                    id: text3
                    text: qsTr("Automatic Collisions")
                    color: "#ffffff"
                    font.pixelSize: 12
                }

                TabletComboBox {
                    id: collisionType
                    width: 200
                    z: 100
                    transformOrigin: Item.Center
                    model: ["No Collision",
                            "Basic - Whole model",
                            "Good - Sub-meshes",
                            "Exact - All polygons"]
                }

                Row {
                    id: row3
                    width: 200
                    height: 400
                    spacing: 5
                    
                    anchors {
                        rightMargin: 15
                    }
                    Button {
                        id: button1
                        text: qsTr("Add")
                        z: -1
                        onClicked: {
                            newModelDialog.sendToScript({
                                method: "newModelDialogAdd",
                                params: {
                                    textInput: modelURL.text,
                                    checkBox: dynamic.checked,
                                    comboBox: collisionType.currentIndex
                                }
                            });
                        }
                    }

                    Button {
                        id: button2
                        z: -1
                        text: qsTr("Cancel")
                        onClicked: {
                            newModelDialog.sendToScript({method: "newModelDialogCancel"})
                        }
                    }
                }
            }
        }
    }
}
