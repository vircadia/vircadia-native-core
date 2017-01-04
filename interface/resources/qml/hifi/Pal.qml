//
//  Pal.qml
//  qml/hifi
//
//  People Action List 
//
//  Created by Howard Stearns on 12/12/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* TODO:

   prototype:
   - only show kick/mute when canKick
   - margins everywhere
   - column head centering
   - column head font
   - proper button .svg on toolbar

   mvp:
   - Show all participants, including ignored, and populate initial ignore/mute status.
   - If name is elided, hover should scroll name left so the full name can be read.
   
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
import "../styles-uit"
import "../controls-uit" as HifiControls

Item {
    id: pal
    // Size
    width: parent.width
    height: parent.height
    // Properties
    property int myCardHeight: 70
    property int rowHeight: 70
    property int actionButtonWidth: 75
    property int nameCardWidth: width - actionButtonWidth*(iAmAdmin ? 4 : 2)

    // This contains the current user's NameCard and will contain other information in the future
    Rectangle {
        id: myInfo
        // Size
        width: pal.width
        height: myCardHeight + 20
        // Anchors
        anchors.top: pal.top
        // Properties
        radius: hifi.dimensions.borderRadius
        // This NameCard refers to the current user's NameCard (the one above the table)
        NameCard {
            id: myCard
            // Properties
            displayName: myData.displayName
            userName: myData.userName
            audioLevel: myData.audioLevel
            // Size
            width: nameCardWidth
            height: parent.height
            // Anchors
            anchors.left: parent.left
        }
    }
    // Rectangles used to cover up rounded edges on bottom of MyInfo Rectangle
    Rectangle {
        color: "#FFFFFF"
        width: pal.width
        height: 10
        anchors.top: myInfo.bottom
        anchors.left: parent.left
    }
    Rectangle {
        color: "#FFFFFF"
        width: pal.width
        height: 10
        anchors.bottom: table.top
        anchors.left: parent.left
    }
    // Rectangle that houses "ADMIN" string
    Rectangle {
        id: adminTab
        // Size
        width: actionButtonWidth * 2 - 2
        height: 40
        // Anchors
        anchors.bottom: myInfo.bottom
        anchors.bottomMargin: -10
        anchors.right: myInfo.right
        // Properties
        visible: iAmAdmin
        // Style
        color: hifi.colors.tableRowLightEven
        radius: hifi.dimensions.borderRadius
        border.color: hifi.colors.lightGrayText
        border.width: 2
        // "ADMIN" text
        RalewaySemiBold {
            text: "ADMIN"
            // Text size
            size: hifi.fontSizes.tableHeading + 2
            // Anchors
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.right: parent.right
            // Style
            font.capitalization: Font.AllUppercase
            color: hifi.colors.redHighlight
            // Alignment
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignTop
        }
    }
    // This TableView refers to the table (below the current user's NameCard)
    HifiControls.Table {
        id: table
        // Size
        height: pal.height - myInfo.height - 4
        width: pal.width - 4
        // Anchors
        anchors.left: parent.left
        anchors.top: myInfo.bottom
        // Properties
        centerHeaderText: true
        sortIndicatorVisible: true
        headerVisible: true
        onSortIndicatorColumnChanged: sortModel()
        onSortIndicatorOrderChanged: sortModel()

        TableViewColumn {
            role: "displayName"
            title: "NAMES"
            width: nameCardWidth
            movable: false
            resizable: false
        }
        TableViewColumn {
            role: "personalMute"
            title: "MUTE"
            width: actionButtonWidth
            movable: false
            resizable: false
        }
        TableViewColumn {
            role: "ignore"
            title: "IGNORE"
            width: actionButtonWidth
            movable: false
            resizable: false
        }
        TableViewColumn {
            visible: iAmAdmin
            role: "mute"
            title: "SILENCE"
            width: actionButtonWidth
            movable: false
            resizable: false
        }
        TableViewColumn {
            visible: iAmAdmin
            role: "kick"
            title: "BAN"
            width: actionButtonWidth
            movable: false
            resizable: false
        }
        model: userModel

        // This Rectangle refers to each Row in the table.
        rowDelegate: Rectangle { // The only way I know to specify a row height.
            // Size
            height: rowHeight
            color: styleData.selected
                   ? "#afafaf"
                   : styleData.alternate ? hifi.colors.tableRowLightEven : hifi.colors.tableRowLightOdd
        }

        // This Item refers to the contents of each Cell
        itemDelegate: Item {
            id: itemCell
            property bool isCheckBox: typeof(styleData.value) === 'boolean'
            // This NameCard refers to the cell that contains an avatar's
            // DisplayName and UserName
            NameCard {
                id: nameCard
                // Properties
                displayName: styleData.value
                userName: model.userName
                audioLevel: model.audioLevel
                visible: !isCheckBox
                // Size
                width: nameCardWidth
                height: parent.height
                // Anchors
                anchors.left: parent.left
            }
            
            // This CheckBox belongs in the columns that contain the action buttons ("Mute", "Ban", etc)
            HifiControls.CheckBox {
                visible: isCheckBox
                anchors.centerIn: parent
                boxSize: 24
                onClicked: {
                    var newValue = !model[styleData.role]
                    var datum = userData[model.userIndex]
                    datum[styleData.role] = model[styleData.role] = newValue
                    Users[styleData.role](model.sessionId)
                    // Just for now, while we cannot undo things:
                    userData.splice(model.userIndex, 1)
                    sortModel()
                }
            }
        }
    }
    // Refresh button
    Rectangle {
        // Size
        width: hifi.dimensions.tableHeaderHeight-1
        height: hifi.dimensions.tableHeaderHeight-1
        // Anchors
        anchors.left: table.left
        anchors.leftMargin: 4
        anchors.top: table.top
        // Style
        color: hifi.colors.tableBackgroundLight
        // Actual refresh icon
        HiFiGlyphs {
            id: reloadButton
            text: hifi.glyphs.reloadSmall
            // Size
            size: parent.width*1.5
            // Anchors
            anchors.fill: parent
            // Style
            horizontalAlignment: Text.AlignHCenter
            color: hifi.colors.darkGray
        }
        MouseArea {
            id: reloadButtonArea
            // Anchors
            anchors.fill: parent
            hoverEnabled: true
            // Everyone likes a responsive refresh button!
            // So use onPressed instead of onClicked
            onPressed: {
                reloadButton.color = hifi.colors.lightGrayText
                pal.sendToScript({method: 'refresh'})
            }
            onReleased: reloadButton.color = (containsMouse ? hifi.colors.baseGrayHighlight : hifi.colors.darkGray)
            onEntered: reloadButton.color = hifi.colors.baseGrayHighlight
            onExited: reloadButton.color = (pressed ?  hifi.colors.lightGrayText: hifi.colors.darkGray)
        }
    }

    // Separator between user and admin functions
    Rectangle {
        // Size
        width: 2
        height: table.height
        // Anchors
        anchors.left: adminTab.left
        anchors.top: table.top
        // Properties
        visible: iAmAdmin
        color: hifi.colors.lightGrayText
    }
    // This Rectangle refers to the [?] popup button
    Rectangle {
        color: hifi.colors.tableBackgroundLight
        width: 20
        height: hifi.dimensions.tableHeaderHeight - 2
        anchors.left: table.left
        anchors.top: table.top
        anchors.topMargin: 1
        anchors.leftMargin: nameCardWidth/2 + 24
        RalewayRegular {
            id: helpText
            text: "[?]"
            size: hifi.fontSizes.tableHeading + 2
            font.capitalization: Font.AllUppercase
            color: hifi.colors.darkGray
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
        }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            hoverEnabled: true
            onClicked: namesPopup.visible = true
            onEntered: helpText.color = hifi.colors.baseGrayHighlight
            onExited: helpText.color = hifi.colors.darkGray
        }
    }
    // Explanitory popup upon clicking "[?]"
    Item {
        visible: false
        id: namesPopup
        anchors.fill: pal
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.5
            radius: hifi.dimensions.borderRadius
        }
        Rectangle {
            width: Math.min(parent.width * 0.75, 400)
            height: popupText.contentHeight*2
            anchors.centerIn: parent
            radius: hifi.dimensions.borderRadius
            color: "white"
            FiraSansSemiBold {
                id: popupText
                text: "This is temporary text. It will eventually be used to explain what 'Names' means."
                size: hifi.fontSizes.textFieldInput
                color: hifi.colors.darkGray
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                wrapMode: Text.WordWrap
            }
        }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                namesPopup.visible = false
            }
        }
    }

    property var userData: []
    property var myData: ({displayName: "", userName: "", audioLevel: 0.0}) // valid dummy until set
    property bool iAmAdmin: false
    function findSessionIndex(sessionId, optionalData) { // no findIndex in .qml
        var i, data = optionalData || userData, length = data.length;
        for (var i = 0; i < length; i++) {
            if (data[i].sessionId === sessionId) {
                return i;
            }
        }
        return -1;
    }
    function fromScript(message) {
        switch (message.method) {
        case 'users':
            var data = message.params;
            var myIndex = findSessionIndex('', data);
            iAmAdmin = Users.canKick;
            myData = data[myIndex];
            data.splice(myIndex, 1);
            userData = data;
            sortModel();
            break;
        case 'select':
            var sessionId = message.params[0];
            var selected = message.params[1];
            var userIndex = findSessionIndex(sessionId);
            if (selected) {
                table.selection.clear(); // for now, no multi-select
                table.selection.select(userIndex);
            } else {
                table.selection.deselect(userIndex);
            }
            break;
        // Received an "updateUsername()" request from the JS
        case 'updateUsername':
            // The User ID (UUID) is the first parameter in the message.
            var userId = message.params[0];
            // The text that goes in the userName field is the second parameter in the message.
            var userName = message.params[1];
            // If the userId is empty, we're updating "myData".
            if (!userId) {
                myData.userName = userName;
                myCard.userName = userName; // Defensive programming
            } else {
                // Get the index in userModel and userData associated with the passed UUID
                var userIndex = findSessionIndex(userId);
                // Set the userName appropriately
                userModel.get(userIndex).userName = userName;
                userData[userIndex].userName = userName; // Defensive programming
            }
            break;
        case 'updateAudioLevel': 
            for (var userId in message.params) {
                var audioLevel = message.params[userId];
                // If the userId is 0, we're updating "myData".
                if (userId == 0) {
                    myData.audioLevel = audioLevel;
                    myCard.audioLevel = audioLevel; // Defensive programming
                } else {
                    console.log("userid:" + userId);
                    var userIndex = findSessionIndex(userId);
                    userModel.get(userIndex).audioLevel = audioLevel;
                    userData[userIndex].audioLevel = audioLevel; // Defensive programming
                }
            }
            break;
        default:
            console.log('Unrecognized message:', JSON.stringify(message));
        }
    }
    ListModel {
        id: userModel
    }
    function sortModel() {
        var sortProperty = table.getColumn(table.sortIndicatorColumn).role;
        var before = (table.sortIndicatorOrder === Qt.AscendingOrder) ? -1 : 1;
        var after = -1 * before;
        userData.sort(function (a, b) {
            var aValue = a[sortProperty].toString().toLowerCase(), bValue = b[sortProperty].toString().toLowerCase();
            switch (true) {
            case (aValue < bValue): return before;
            case (aValue > bValue): return after;
            default: return 0;
            }
        });
        table.selection.clear();
        userModel.clear();
        var userIndex = 0;
        userData.forEach(function (datum) {
            function init(property) {
                if (datum[property] === undefined) {
                    datum[property] = false;
                }
            }
            ['personalMute', 'ignore', 'mute', 'kick'].forEach(init);
            datum.userIndex = userIndex++;
            userModel.append(datum);
        });
    }
    signal sendToScript(var message);
    function noticeSelection() {
        var userIds = [];
        table.selection.forEach(function (userIndex) {
            userIds.push(userData[userIndex].sessionId);
        });
        pal.sendToScript({method: 'selected', params: userIds});
    }
    Connections {
        target: table.selection
        onSelectionChanged: pal.noticeSelection()
    }
}
