//
//  NewModelDialog.qml
//  qml/hifi
//
//  Created by Seth Alves on 2017-2-10
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Dialogs 1.2 as OriginalDialogs

import stylesUit 1.0
import controlsUit 1.0
import hifi.dialogs 1.0

Rectangle {
    id: newModelDialog
    // width: parent.width
    // height: parent.height
    HifiConstants { id: hifi }
    color: hifi.colors.baseGray;
    signal sendToScript(var message);
    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property bool keyboardRasied: false

    function errorMessageBox(message) {
        try {
            return desktop.messageBox({
                icon: hifi.icons.warning,
                defaultButton: OriginalDialogs.StandardButton.Ok,
                title: "Error",
                text: message
            });
        } catch(e) {
            Window.alert(message);
        }
    }

    Item {
        id: column1
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        anchors.bottomMargin: 10
        anchors.topMargin: 10
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: keyboard.top

        Text {
            id: text1
            text: qsTr("Model URL <i>(.fbx, .fst, .glb, .gltf, .obj, .gz)</i>")
            color: "#ffffff"
            font.pixelSize: 12
        }

        TextInput {
            id: modelURL
            height: 20
            text: qsTr("")
            color: "white"
            anchors.top: text1.bottom
            anchors.topMargin: 5
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 0
            font.pixelSize: 12

            onAccepted: {
                newModelDialog.keyboardEnabled = false;
            }

            onTextChanged : {
                if (modelURL.text.length === 0){
                    button1.enabled = false;
                } else {
                    button1.enabled = true;
                }
            }
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    newModelDialog.keyboardEnabled = HMD.active
                    parent.focus = true;
                    parent.forceActiveFocus();
                    modelURL.cursorPosition = modelURL.positionAt(mouseX, mouseY, TextInput.CursorBetweenCharaters);
                }
            }
        }

        Rectangle {
            id: textInputBox
            color: "white"
            anchors.fill: modelURL
            opacity: 0.1
        }

        Row {
            id: row1
            height: 400
            spacing: 30
            anchors.top: modelURL.top
            anchors.topMargin: 25
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 0

            Column {
                id: column2
                width: 200
                height: 600
                spacing: 10

                CheckBox {
                    id: useOriginalPivot
                    text: qsTr("Use Original Pivot")
                    checked: true
                }

                CheckBox {
                    id: grabbable
                    text: qsTr("Grabbable")
                }

                CheckBox {
                    id: dynamic
                    text: qsTr("Dynamic")
                }

                Row {
                    id: row2
                    width: 200
                    height: 400
                    spacing: 20

                    Text {
                        id: text2
                        width: 160
                        x: dynamic.width / 2
                        color: "#ffffff"
                        text: qsTr("Models with automatic collisions set to 'Exact' cannot be dynamic.")
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

                ComboBox {
                    id: collisionType

                    property int priorIndex: 0
                    property string staticMeshCollisionText: "Exact - All polygons"
                    property var collisionArray: ["No Collision",
                                                  "Basic - Whole model",
                                                  "Good - Sub-meshes",
                                                  staticMeshCollisionText, 
                                                  "Box", 
                                                  "Sphere"]

                    width: 200
                    z: 100
                    transformOrigin: Item.Center
                    model: collisionArray

                    onCurrentIndexChanged: {
                        if (collisionArray[currentIndex] === staticMeshCollisionText) {
                            
                            if (dynamic.checked) {
                                currentIndex = priorIndex;

                                errorMessageBox("Models with Automatic Collisions set to \"" 
                                                    + staticMeshCollisionText + "\" cannot be dynamic.");
                                //--EARLY EXIT--( Can't have a static mesh model that's dynamic )
                                return;
                            }

                            dynamic.enabled = false;
                        } else {
                            dynamic.enabled = true;
                        }

                        priorIndex = currentIndex;
                    }
                }

                Row {
                    id: row3
                    width: 200
                    height: 400
                    spacing: 5

                    anchors.horizontalCenter: column3.horizontalCenter
                    anchors.horizontalCenterOffset: -20

                    Button {
                        id: button1
                        text: qsTr("Create")
                        z: -1
                        enabled: false
                        onClicked: {
                            newModelDialog.sendToScript({
                                method: "newModelDialogAdd",
                                params: {
                                    url: modelURL.text,
                                    dynamic: dynamic.checked,
                                    collisionShapeIndex: collisionType.currentIndex,
                                    grabbable: grabbable.checked,
                                    useOriginalPivot: useOriginalPivot.checked
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

    Keyboard {
        id: keyboard
        raised: parent.keyboardEnabled && parent.keyboardRaised
        numeric: parent.punctuationMode
        anchors {
            bottom: parent.bottom
            bottomMargin: 40
            left: parent.left
            right: parent.right
        }
    }
}
