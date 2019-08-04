//
//  EmojiApp.qml
//
//  Created by Milad Nazeri on 2019-08-03
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import "../simplifiedConstants" as SimplifiedConstants
import "../simplifiedControls" as SimplifiedControls
import stylesUit 1.0 as HifiStylesUit
import "./emojiList.js" as EmojiList
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.
import TabletScriptingInterface 1.0

Rectangle {
    id: root
    color: simplifiedUI.colors.darkBackground
    // color: simplifiedUI.colors.darkBackground
    anchors.fill: parent
    
    readonly property string emojiBaseURL: "./images/emojis/png1024/"
    readonly property string emojiSpriteBaseURL: "./images/emojis/" 
    property string currentCode: ""
    property bool emojiRunning: false

    onCurrentCodeChanged: {
        console.log("CURRENT CODE IS BEING CHANGED");
        mainEmojiImage.source = emojiBaseURL + currentCode + ".png";
        console.log("new main emoji image source:", mainEmojiImage.source);
    }

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    focus: true

    ListModel {
        id: mainModel
    }

    Component.onCompleted: {
        root.forceActiveFocus();
        EmojiList.emojiList
            .filter( emoji => {
                return emoji.mainCategory === "Smileys & Emotion" || 
                emoji.mainCategory === "People & Body" ||
                emoji.mainCategory === "Animals & Nature" ||
                emoji.mainCategory === "Food & Drink";
            })
            .forEach(function(item, index){
                item.code = { utf: item.code[0] }
                item.keywords = { keywords: item.keywords }
                mainModel.append(item);
            });
            root.currentCode = mainModel.get(0).code.utf;
            console.log("CURRENT CODE", root.currentCode);
    }

    Rectangle {
        id: emojiIndicatorContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 200
        clip: true
        color: simplifiedUI.colors.darkBackground

        Image {
            id: mainEmojiImage
            width: 180
            height: 180
            anchors.centerIn: parent
            source: emojiBaseURL + root.currentCode + ".png"
            fillMode: Image.PreserveAspectFit
        }
    }


    Rectangle {
        id: emojiIconListContainer
        anchors.top: emojiIndicatorContainer.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 415
        clip: true
        color: simplifiedUI.colors.darkBackground
        // color: "#DDFF33"

        // TODO: Fix the margin in the emoji, you can tell from the highlight
        Component {
            id: emojiDelegate
            Item {
                width: grid.cellWidth; height: grid.cellHeight
                Column {
                    Rectangle {
                        id: imageContainer
                        // anchors.centerIn: emojiDelegate.centerIn
                        clip: true; 
                        z:1
                        width: 36
                        height: 36
                        x: 2
                        y: -2
                        color: Qt.rgba(1, 1, 1, 0.0)
                        Image {
                            source: emojiSpriteBaseURL + normal.source
                            z: 2
                            fillMode: Image.Pad 
                            x: -normal.frame.x
                            y: -normal.frame.y
                            MouseArea {
                                hoverEnabled: enabled
                                anchors.fill: parent
                                onEntered: {
                                    // Tablet.playSound(TabletEnums.ButtonClick);
                                    grid.currentIndex = index
                                    console.log("model.get(grid.currentIndex).code.utf", mainModel.get(index).code.utf)
                                    root.currentCode = mainModel.get(index).code.utf;
                                }
                                onClicked: {
                                    console.log("GOT THE CLICK on emoji");
                                    sendToScript({
                                        "source": "EmoteAppBar.qml",
                                        "method": "selectedEmoji",
                                        "code": code.utf
                                    });
                                }
                                onExited: {
                                }
                            }
                        }
                    }                
                }
            }
            
        }

        GridView {
            id: grid
            anchors.fill: parent
            anchors.centerIn: parent
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            width: 480
            height: 415
            cellWidth: 40
            cellHeight: 40
            model: mainModel
            delegate: emojiDelegate
            focus: true
            highlight: Rectangle { color: Qt.rgba(1, 1, 1, 0.4); radius: 0 }
        }
    }
    signal sendToScript(var message);

}