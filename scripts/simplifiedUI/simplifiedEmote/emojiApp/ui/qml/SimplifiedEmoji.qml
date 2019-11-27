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
import QtGraphicalEffects 1.12
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0
import hifi.simplifiedUI.simplifiedControls 1.0 as SimplifiedControls
import hifi.simplifiedUI.simplifiedConstants 1.0 as SimplifiedConstants
import "../../resources/modules/emojiList.js" as EmojiList
import "../../resources/modules/customEmojiList.js" as CustomEmojiList

Rectangle {
    id: root
    color: simplifiedUI.colors.darkBackground
    anchors.fill: parent
    
    // Used for the indicator picture
    readonly property string emojiBaseURL: "../../resources/images/emojis/512px/"
    readonly property string emoji52BaseURL: "../../resources/images/emojis/52px/"
    // Capture the selected code to handle which emoji to show
    property string currentCode: ""
    // if this is true, then hovering doesn't allow showing other icons
    property bool isSelected: false

    KeyNavigation.backtab: emojiSearchTextField
    KeyNavigation.tab: emojiSearchTextField

    // Update the selected emoji image whenever the code property is changed.
    onCurrentCodeChanged: {
        mainEmojiImage.source = emojiBaseURL + currentCode;
    }

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    focus: true

    ListModel {
        id: mainModel
    }

    ListModel {
        id: filteredModel
    }

    Component.onCompleted: {
        emojiSearchTextField.forceActiveFocus();
        EmojiList.emojiList
            .filter(emoji => {
                return emoji.mainCategory === "Smileys & Emotion" || 
                emoji.mainCategory === "People & Body" ||
                emoji.mainCategory === "Animals & Nature" ||
                emoji.mainCategory === "Food & Drink";
            })
            // Convert the filtered list to seed our QML Model used for our view
            .forEach(function(item, index){
                item.code = { utf: item.code[0] }
                item.keywords = { keywords: item.keywords }
                item.shortName = item.shortName
                mainModel.append(item);
                filteredModel.append(item);
            });
        CustomEmojiList.customEmojiList
            .forEach(function(item, index){
                item.code = { utf: item.name }
                item.keywords = { keywords: item.keywords }
                item.shortName = item.name
                mainModel.append(item);
                filteredModel.append(item);
            });
            
        root.currentCode = filteredModel.get(0).code.utf;
    }

    Rectangle {
        id: popupContainer
        z: 999
        visible: false
        color: Qt.rgba(0, 0, 0, 0.8)
        anchors.fill: parent
            
        MouseArea {
            hoverEnabled: false
            anchors.fill: parent
            onClicked: {
                popupContainer.visible = false;
            }
        }
            
        MouseArea {
            hoverEnabled: false
            anchors.fill: popupContentContainer
            propagateComposedEvents: false
        }

        Rectangle {
            id: popupContentContainer
            color: simplifiedUI.colors.darkBackground
            anchors.centerIn: parent
            width: 300
            height: 500

            Item {
                id: closeButtonContainer
                anchors.top: parent.top
                anchors.right: parent.right
                width: 50
                height: width

                HifiStylesUit.GraphikSemiBold {
                    text: "X"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: simplifiedUI.colors.text.darkGrey
                    opacity: closeButtonMouseArea.containsMouse ? 1.0 : 0.8
                    size: 22
                }
            
                MouseArea {
                    id: closeButtonMouseArea
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: {
                        popupContainer.visible = false;
                    }
                }
            }

            HifiStylesUit.GraphikSemiBold {
                id: popupHeader1
                text: "Emoji Reactions"
                anchors.top: parent.top
                anchors.topMargin: 10
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: parent.right
                anchors.rightMargin: 16
                width: paintedWidth
                height: 24
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignTop
                color: simplifiedUI.colors.text.white
                size: 22
            }

            HifiStylesUit.GraphikRegular {
                id: popupText1
                text: "Click an emoji and it will appear above your head!\nUse Emoji Reactions to express yourself without saying a word.\n\n" +
                    "Try using the search bar to search for an emoji directly. You can use your arrow keys to navigate the results."
                anchors.top: popupHeader1.bottom
                anchors.topMargin: 16
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: parent.right
                anchors.rightMargin: 16
                height: paintedHeight
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignTop
                color: simplifiedUI.colors.text.white
                size: 18
            }

            HifiStylesUit.GraphikSemiBold {
                id: popupHeader2
                text: "Attributions"
                anchors.top: popupText1.bottom
                anchors.topMargin: 16
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: parent.right
                anchors.rightMargin: 16
                height: 24
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignTop
                color: simplifiedUI.colors.text.white
                size: 22
            }

            HifiStylesUit.GraphikRegular {
                id: popupText2
                textFormat: Text.RichText
                text: "<style>a:link { color: #FFF; }</style>" +
                    "Emoji Reaction images are <a href='https://twemoji.twitter.com/content/twemoji-twitter/en.html'>Twemojis</a> " +
                    "and are © 2019 Twitter, Inc and other contributors.\n" +
                    "Graphics are licensed under <a href='https://creativecommons.org/licenses/by/4.0/'>CC-BY 4.0</a>."
                anchors.top: popupHeader2.bottom
                anchors.topMargin: 16
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: parent.right
                anchors.rightMargin: 16
                height: paintedHeight
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignTop
                color: simplifiedUI.colors.text.white
                size: 18

                onLinkActivated: {
                    Qt.openUrlExternally(link);
                }
            }
        }
    }
    
    // If this MouseArea is hit, the user is clicking on an area not handled
    // by any other MouseArea
    MouseArea {
        anchors.fill: parent
        onClicked: {
            // The grid will forward keypresses to the main Interface window
            grid.forceActiveFocus();
            // Necessary to get keyboard keyPress/keyRelease forwarding to work. See `Application::hasFocus()`;
            Window.setFocus();
        }
    }

    Rectangle {
        id: emojiIndicatorContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 160
        clip: true
        color: simplifiedUI.colors.darkBackground

        Image {
            id: mainEmojiLowOpacity
            width: mainEmojiImage.width
            height: mainEmojiImage.height
            anchors.centerIn: parent
            source: mainEmojiImage.source
            opacity: 0.5
            fillMode: Image.PreserveAspectFit
            visible: true
            mipmap: true
        }

        Image {
            id: mainEmojiImage
            width: emojiIndicatorContainer.width - 20
            height: emojiIndicatorContainer.height - 20
            anchors.centerIn: parent
            source: ""
            fillMode: Image.PreserveAspectFit
            visible: false
            mipmap: true
        }

        // The overlay used during the pie timeout
        SimplifiedControls.ProgressCircle {
            id: progressCircle
            animationDuration: 7000 // Must match `TOTAL_EMOJI_DURATION_MS` in `simplifiedEmoji.js`
            anchors.centerIn: mainEmojiImage
            size: mainEmojiImage.width * 2
            opacity: 0.5
            colorCircle: "#FFFFFF"
            colorBackground: "#E6E6E6"
            showBackground: false
            isPie: true
            arcBegin: 0
            arcEnd: 360
            visible: false
        }

        OpacityMask {
            anchors.fill: mainEmojiImage
            source: mainEmojiImage
            maskSource: progressCircle
        }
    }


    Item {
        id: searchBarContainer
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: emojiIndicatorContainer.bottom
        width: Math.min(parent.width, 420)
        height: 48

        SimplifiedControls.TextField {
            id: emojiSearchTextField
            readonly property string defaultPlaceholderText: "Search Emojis"
            bottomBorderVisible: false
            backgroundColor: "#313131"
            placeholderText: emojiSearchTextField.defaultPlaceholderText
            maximumLength: 100
            clip: true
            selectByMouse: true
            autoScroll: true
            horizontalAlignment: TextInput.AlignHCenter
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            onTextChanged: {
                if (text.length === 0) {
                    root.filterEmoji(emojiSearchTextField.text);
                } else {
                    waitForMoreInputTimer.restart();
                }
            }
            onAccepted: {
                root.filterEmoji(emojiSearchTextField.text);
                waitForMoreInputTimer.stop();
                if (filteredModel.count === 1) {
                    root.selectEmoji(filteredModel.get(0).code.utf);
                } else {
                    grid.forceActiveFocus();
                }
            }

            KeyNavigation.backtab: grid
            KeyNavigation.tab: grid
        }

        Timer {
            id: waitForMoreInputTimer
            repeat: false
            running: false
            triggeredOnStart: false
            interval: 300

            onTriggered: {
                root.filterEmoji(emojiSearchTextField.text);
            }
        }
    }


    function selectEmoji(code) {
        sendToScript({
            "source": "SimplifiedEmoji.qml",
            "method": "selectedEmoji",
            "code": code
        });
        root.isSelected = true;
        root.currentCode = code;
    }


    Rectangle {
        id: emojiIconListContainer
        anchors.top: searchBarContainer.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        clip: true
        color: simplifiedUI.colors.darkBackground

        Item {
            id: helpGlyphContainer
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 2
            width: 22
            height: width

            HifiStylesUit.GraphikRegular {
                text: "?"
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: simplifiedUI.colors.text.darkGrey
                opacity: attributionMouseArea.containsMouse ? 1.0 : 0.8
                size: 22
            }
        
            MouseArea {
                id: attributionMouseArea
                hoverEnabled: enabled
                anchors.fill: parent
                propagateComposedEvents: false
                onClicked: {
                    popupContainer.visible = true;
                }
            }
        }

        GridView {
            id: grid
            anchors.fill: parent
            anchors.leftMargin: 30
            anchors.rightMargin: 24
            cellWidth: 60
            cellHeight: 60
            model: filteredModel
            delegate: Image {
                    width: 52
                    height: 52
                    source: emoji52BaseURL + model.code.utf
                    fillMode: Image.PreserveAspectFit
                    MouseArea {
                        hoverEnabled: enabled
                        anchors.fill: parent
                        propagateComposedEvents: false
                        onEntered: {
                            grid.currentIndex = index;
                            // don't allow a hover image change of the main emoji image 
                            if (root.isSelected) {
                                return;
                            }
                            // Updates the selected image
                            root.currentCode = model.code.utf;
                            // Ensures that the placeholder text is visible and updated
                            if (emojiSearchTextField.text === "") {
                                grid.forceActiveFocus();
                            }
                            emojiSearchTextField.placeholderText = "::" + model.shortName + "::";
                        }
                        onClicked: {
                            root.selectEmoji(model.code.utf);
                            // The grid will forward keypresses to the main Interface window
                            grid.forceActiveFocus();
                            // Necessary to get keyboard keyPress/keyRelease forwarding to work. See `Application::hasFocus()`;
                            Window.setFocus();
                        }
                    }
                }
            cacheBuffer: 400
            focus: true
            highlight: Rectangle {
                color: Qt.rgba(1, 1, 1, 0.4)
                radius: 2
            }

            KeyNavigation.backtab: emojiSearchTextField
            KeyNavigation.tab: emojiSearchTextField

            Keys.onPressed: {
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    root.selectEmoji(grid.model.get(grid.currentIndex).code.utf);
                }
                root.keyPressEvent(event.key, event.modifiers);
            }
            Keys.onReleased: {
                root.keyReleaseEvent(event.key, event.modifiers);
            }
        }

        SimplifiedControls.VerticalScrollBar {
            parent: grid
            anchors.rightMargin: -grid.anchors.rightMargin + 2
        }

        HifiStylesUit.GraphikRegular {
            readonly property var cantFindEmojiList: ["😣", "😭", "😖", "😢", "🤔"]
            onVisibleChanged: {
                if (visible) {
                    text = "We couldn't find that emoji " + cantFindEmojiList[Math.floor(Math.random() * cantFindEmojiList.length)]
                }
            }
            visible: grid.model.count === 0
            anchors.fill: parent
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: simplifiedUI.colors.text.darkGrey
            size: 22
        }
    }

    function filterEmoji(filterText) {
        filteredModel.clear();

        if (filterText.length === 0) {
            for (var i = 0; i < mainModel.count; i++) {
                filteredModel.append(mainModel.get(i));
            }
            return;
        }

        for (var i = 0; i < mainModel.count; i++) {
            var currentObject = mainModel.get(i);
            var currentKeywords = currentObject.keywords.keywords;
            for (var j = 0; j < currentKeywords.length; j++) {
                if ((currentKeywords[j].toLowerCase()).indexOf(filterText.toLowerCase()) > -1) {
                    filteredModel.append(mainModel.get(i));
                    break;
                }
            }
        }
    }

    signal sendToScript(var message)
    signal keyPressEvent(int key, int modifiers)
    signal keyReleaseEvent(int key, int modifiers)
    Keys.onPressed: {
        root.keyPressEvent(event.key, event.modifiers);
    }
    Keys.onReleased: {
        root.keyReleaseEvent(event.key, event.modifiers);
    }

    function fromScript(message) {
        if (message.source !== "simplifiedEmoji.js") {
            return;
        }

        switch(message.method) {
            case "beginCountdownTimer":
                progressCircle.endAnimation = true;
                progressCircle.arcEnd = 0;
                root.isSelected = true;
            break;
            case "clearCountdownTimer":
                progressCircle.endAnimation = false;
                progressCircle.arcEnd = 360;
                progressCircle.endAnimation = true;
                root.isSelected = false;
            break;
            default:
                console.log("Message not recognized from simplifiedEmoji.js", JSON.stringify(message));
        }
    }
}