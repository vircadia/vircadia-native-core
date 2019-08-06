//
//  SimplifiedEmoji.qml
//
//  Created by Milad Nazeri on 2019-08-03
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.12
import QtQuick.Controls 2.4
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0
/*
    See note in SimplifiedEmoteIndicator about import question
*/
import "qrc:////qml//hifi//simplifiedUI//simplifiedConstants" as SimplifiedConstants
import "../../resources/modules/emojiList.js" as EmojiList
import "./ProgressCircle"

Rectangle {
    id: root
    color: simplifiedUI.colors.darkBackground
    anchors.fill: parent
    
    // Used for the indicator picture
    readonly property string emojiBaseURL: "../../resources/images/emojis/png1024/"
    // Sprite sheet used for the smaller icons
    readonly property string emojiSpriteBaseURL: "../../resources/images/emojis/"
    // Capture the selected code to handle which emoji to show
    property string currentCode: ""
    // if this is true, then hovering doesn't allow showing other icons
    property bool isSelected: false

    // Update the selected emoji image whenever the code property is changed.
    onCurrentCodeChanged: {
        mainEmojiImage.source = emojiBaseURL + currentCode + ".png";
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
        /*
            The emoji list we have is a node transformed list of all the UTF emojis with meta info.
            To cut down on the list, this is a good place to start as they will be 90% of the emojis anyone would
            want to use.

            After we cut
        */
        EmojiList.emojiList
            .filter( emoji => {
                return emoji.mainCategory === "Smileys & Emotion" || 
                emoji.mainCategory === "People & Body" ||
                emoji.mainCategory === "Animals & Nature" ||
                emoji.mainCategory === "Food & Drink";
            })
            // Convert the filtered list to seed our QML Model used for our view
            .forEach(function(item, index){
                item.code = { utf: item.code[0] }
                item.keywords = { keywords: item.keywords }
                mainModel.append(item);
            });
            // Deleting this might remove any emoji that is shown when you open the app
            // Keeping in case the spec design might prefer this instead
            root.currentCode = mainModel.get(0).code.utf;
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
            source: ""
            fillMode: Image.PreserveAspectFit
        }

        // The overlay used during the pie timeout
        ProgressCircle {
            id: progressCircle
            anchors.centerIn: parent
            size: 180
            opacity: 0.5
            colorCircle: "#FFFFFF"
            colorBackground: "#E6E6E6"
            showBackground: false
            isPie: true
            arcBegin: 0
            arcEnd: 0
        }
    }

    // The bottom half of the app. We might want to do something about the set height to be more responsive
    // Zach question probably.  
    Rectangle {
        id: emojiIconListContainer
        anchors.top: emojiIndicatorContainer.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 415
        clip: true
        color: simplifiedUI.colors.darkBackground


        /*
            This is what the individual view will be.  It's a bit complicated, but it works.  Might not be worth
            refactoring, but might want to check with Zach if he has an issue with below. 
            Here is reasoning / logic: 

            The icons displayed are implemented with a sprite sheet from the previous HTML version.  
            QML seems to have something that lets you play spritesheet animations, but it doesn't have something
            that is like background-position for css.  

            The way I had to recreate it was to use an Image that has no set size / height and to make sure nothing is transformed
            to fit a container.

            Then I have to put that in a rectangle that acts like a mask.  The x,y position of the image on the emoji meta data is what
            is used to move the image x and y itself.  The emoji images per sprite shell are 36 x 36.  In grid view, there are no spacing
            options besides making the cell width and grid have fake extra padding.  I can't make that rectangle that is the mask to be larger than 36 by 36.
            What worked the best was to make another container rectangle the width of the cell, then positioned the emoji mask rectangle to be centered in that.

            The gridview has a function to handle the highlighting which is nice because it comes with an animation already that looks good out the box.
            This covers the whole 40X40 cell space.
        
        */
        Component {
            id: emojiDelegate
            Item {
                width: grid.cellWidth; height: grid.cellHeight
                Column {
                    Rectangle {
                        width: 40
                        height: 40
                        color: Qt.rgba(1, 1, 1, 0.0)
                        Rectangle {
                            id: imageContainer
                            anchors.centerIn: parent
                            clip: true; 
                            z:1
                            width: 36
                            height: 36
                            anchors.leftMargin: 2
                            anchors.topMargin: 2
                            x: 1
                            y: -1
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
                                        grid.currentIndex = index
                                        // don't allow a hover image change of the main emoji image
                                        if (root.isSelected) {
                                            return;
                                        }
                                        // Updates the selected image
                                        root.currentCode = mainModel.get(index).code.utf;
                                    }
                                    onClicked: {
                                        console.log("GOT THE CLICK on emoji");
                                        sendToScript({
                                            "source": "SimplifiedEmoji.qml",
                                            "method": "selectedEmoji",
                                            "code": code.utf
                                        });
                                        root.isSelected = true;
                                        root.currentCode = mainModel.get(index).code.utf;
                                    }
                                    onExited: {
                                    }
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

    function fromScript(message) {
        if (message.source !== "simplifiedEmoji.js") {
            return;
        }

        switch(message.method) {
            case "updateArchEnd":
                progressCircle.arcEnd = message.data.archEnd;
            break;
            case "isSelected":
                root.isSelected = message.data.isSelected
            default:
                console.log("Message not recoganized from simplifiedEmoji.js");
        }
    }
}