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

import QtQuick 2.5
import QtQuick.Controls 1.4
import "../styles-uit"
import "../controls-uit" as HifiControls

Rectangle {
    id: pal
    // Size
    width: parent.width
    height: parent.height
    // Style
    color: "#E3E3E3"
    // Properties
    property int myCardHeight: 70
    property int rowHeight: 70
    property int actionButtonWidth: 75
    property int nameCardWidth: palContainer.width - actionButtonWidth*(iAmAdmin ? 4 : 2) - 4 - hifi.dimensions.scrollbarBackgroundWidth
    property var myData: ({displayName: "", userName: "", audioLevel: 0.0}) // valid dummy until set
    property var ignored: ({}); // Keep a local list of ignored avatars & their data. Necessary because HashMap is slow to respond after ignoring.
    property var userModelData: [] // This simple list is essentially a mirror of the userModel listModel without all the extra complexities.
    property bool iAmAdmin: false

    // This is the container for the PAL
    Rectangle {
        id: palContainer
        // Size
        width: pal.width - 50
        height: pal.height - 50
        // Style
        color: pal.color
        // Anchors
        anchors.centerIn: pal
        // Properties
        radius: hifi.dimensions.borderRadius

    // This contains the current user's NameCard and will contain other information in the future
    Rectangle {
        id: myInfo
        // Size
        width: palContainer.width
        height: myCardHeight + 20
        // Style
        color: pal.color
        // Anchors
        anchors.top: palContainer.top
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
        color: pal.color
        width: palContainer.width
        height: 10
        anchors.top: myInfo.bottom
        anchors.left: parent.left
    }
    Rectangle {
        color: pal.color
        width: palContainer.width
        height: 10
        anchors.bottom: table.top
        anchors.left: parent.left
    }
    // Rectangle that houses "ADMIN" string
    Rectangle {
        id: adminTab
        // Size
        width: 2*actionButtonWidth + hifi.dimensions.scrollbarBackgroundWidth + 2
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
            id: adminTabText
            text: "ADMIN"
            // Text size
            size: hifi.fontSizes.tableHeading + 2
            // Anchors
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: hifi.dimensions.scrollbarBackgroundWidth
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
        height: palContainer.height - myInfo.height - 4
        width: palContainer.width - 4
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
            // The hacky spaces used to center text over the button, since I don't know how to apply a margin
            // to column header text.
            title: "BAN"
            width: actionButtonWidth
            movable: false
            resizable: false
        }
        model: ListModel {
            id: userModel
        }

        // This Rectangle refers to each Row in the table.
        rowDelegate: Rectangle { // The only way I know to specify a row height.
            // Size
            height: rowHeight
            color: styleData.selected
                   ? hifi.colors.orangeHighlight
                   : styleData.alternate ? hifi.colors.tableRowLightEven : hifi.colors.tableRowLightOdd
        }

        // This Item refers to the contents of each Cell
        itemDelegate: Item {
            id: itemCell
            property bool isCheckBox: styleData.role === "personalMute" || styleData.role === "ignore"
            property bool isButton: styleData.role === "mute" || styleData.role === "kick"
            // This NameCard refers to the cell that contains an avatar's
            // DisplayName and UserName
            NameCard {
                id: nameCard
                // Properties
                displayName: styleData.value
                userName: model && model.userName
                audioLevel: model && model.audioLevel
                visible: !isCheckBox && !isButton
                // Size
                width: nameCardWidth
                height: parent.height
                // Anchors
                anchors.left: parent.left
            }
            
            // This CheckBox belongs in the columns that contain the stateful action buttons ("Mute" & "Ignore" for now)
            // KNOWN BUG with the Checkboxes: When clicking in the center of the sorting header, the checkbox
            // will appear in the "hovered" state. Hovering over the checkbox will fix it.
            // Clicking on the sides of the sorting header doesn't cause this problem.
            // I'm guessing this is a QT bug and not anything I can fix. I spent too long trying to work around it...
            // I'm just going to leave the minor visual bug in.
            HifiControls.CheckBox {
                id: actionCheckBox
                visible: isCheckBox
                anchors.centerIn: parent
                checked: model[styleData.role]
                // If this is a "Personal Mute" checkbox, disable the checkbox if the "Ignore" checkbox is checked.
                enabled: !(styleData.role === "personalMute" && model["ignore"])
                boxSize: 24
                onClicked: {
                    var newValue = !model[styleData.role]
                    userModel.setProperty(model.userIndex, styleData.role, newValue)
                    userModelData[model.userIndex][styleData.role] = newValue // Defensive programming
                    Users[styleData.role](model.sessionId, newValue)
                    if (styleData.role === "ignore") {
                        userModel.setProperty(model.userIndex, "personalMute", newValue)
                        userModelData[model.userIndex]["personalMute"] = newValue // Defensive programming
                        if (newValue) {
                            ignored[model.sessionId] = userModelData[model.userIndex]
                        } else {
                            delete ignored[model.sessionId]
                        }
                    }
                    // http://doc.qt.io/qt-5/qtqml-syntax-propertybinding.html#creating-property-bindings-from-javascript
                    // I'm using an explicit binding here because clicking a checkbox breaks the implicit binding as set by
                    // "checked:" statement above.
                    checked = Qt.binding(function() { return (model[styleData.role])})
                }
            }
            
            // This Button belongs in the columns that contain the stateless action buttons ("Silence" & "Ban" for now)
            HifiControls.Button {
                id: actionButton
                color: 2 // Red
                visible: isButton
                anchors.centerIn: parent
                width: 32
                height: 24
                onClicked: {
                    Users[styleData.role](model.sessionId)
                    if (styleData.role === "kick") {
                        // Just for now, while we cannot undo "Ban":
                        userModel.remove(model.userIndex)
                        delete userModelData[model.userIndex] // Defensive programming
                        sortModel()
                    }
                }
                // muted/error glyphs
                HiFiGlyphs {
                    text: (styleData.role === "kick") ? hifi.glyphs.error : hifi.glyphs.muted
                    // Size
                    size: parent.height*1.3
                    // Anchors
                    anchors.fill: parent
                    // Style
                    horizontalAlignment: Text.AlignHCenter
                    color: enabled ? hifi.buttons.textColor[actionButton.color]
                                   : hifi.buttons.disabledTextColor[actionButton.colorScheme]
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
    function letterbox(message) {
        letterboxMessage.text = message;
        letterboxMessage.visible = true

    }
    // This Rectangle refers to the [?] popup button next to "NAMES"
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
            onClicked: letterbox("Bold names in the list are Avatar Display Names.\n" +
                                 "If a Display Name isn't set, a unique Session Display Name is assigned." +
                                 "\n\nAdministrators of this domain can also see the Username or Machine ID associated with each avatar present.")
            onEntered: helpText.color = hifi.colors.baseGrayHighlight
            onExited: helpText.color = hifi.colors.darkGray
        }
    }
    // This Rectangle refers to the [?] popup button next to "ADMIN"
    Rectangle {
        visible: iAmAdmin
        color: adminTab.color
        width: 20
        height: 28
        anchors.right: adminTab.right
        anchors.rightMargin: 31 + hifi.dimensions.scrollbarBackgroundWidth
        anchors.top: adminTab.top
        anchors.topMargin: 2
        RalewayRegular {
            id: adminHelpText
            text: "[?]"
            size: hifi.fontSizes.tableHeading + 2
            font.capitalization: Font.AllUppercase
            color: hifi.colors.redHighlight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
        }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            hoverEnabled: true
            onClicked: letterbox('Silencing a user mutes their microphone. Silenced users can unmute themselves by clicking the "UNMUTE" button on their HUD.\n\n' +
                                 "Banning a user will remove them from this domain and prevent them from returning. You can un-ban users from your domain's settings page.)")
            onEntered: adminHelpText.color = "#94132e"
            onExited: adminHelpText.color = hifi.colors.redHighlight
        }
    }
    LetterboxMessage {
        id: letterboxMessage
    }
    }

    function findSessionIndex(sessionId, optionalData) { // no findIndex in .qml
        var data = optionalData || userModelData, length = data.length;
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
            var index = -1;
            index = findSessionIndex('', data);
            if (index !== -1) {
                iAmAdmin = Users.canKick;
                myData = data[index];
                data.splice(index, 1);
            } else {
                console.log("This user's data was not found in the user list. PAL will not function properly.");
            }
            userModelData = data;
            for (var ignoredID in ignored) {
                index = findSessionIndex(ignoredID);
                if (index === -1) { // Add back any missing ignored to the PAL, because they sometimes take a moment to show up.
                    userModelData.push(ignored[ignoredID]);
                } else { // Already appears in PAL; update properties of existing element in model data
                    userModelData[index] = ignored[ignoredID];
                }
            }
            sortModel();
            break;
        case 'select':
            var sessionIds = message.params[0];
            var selected = message.params[1];
            var userIndex = findSessionIndex(sessionIds[0]);
            if (sessionIds.length > 1) {
                letterbox('Only one user can be selected at a time.');
            } else if (userIndex < 0) {
                letterbox('The last editor is not among this list of users.');
            } else {
                if (selected) {
                    table.selection.clear(); // for now, no multi-select
                    table.selection.select(userIndex);
                    table.positionViewAtRow(userIndex, ListView.Visible);
                } else {
                    table.selection.deselect(userIndex);
                }
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
                // Get the index in userModel and userModelData associated with the passed UUID
                var userIndex = findSessionIndex(userId);
                if (userIndex != -1) {
                    // Set the userName appropriately
                    userModel.setProperty(userIndex, "userName", userName);
                    userModelData[userIndex].userName = userName; // Defensive programming
                } else {
                    console.log("updateUsername() called with unknown UUID: ", userId);
                }
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
                    var userIndex = findSessionIndex(userId);
                    if (userIndex != -1) {
                        userModel.setProperty(userIndex, "audioLevel", audioLevel);
                        userModelData[userIndex].audioLevel = audioLevel; // Defensive programming
                    } else {
                        console.log("updateUsername() called with unknown UUID: ", userId);
                    }
                }
            }
            break;
        case 'clearIgnored': 
            ignored = {};
            break;
        default:
            console.log('Unrecognized message:', JSON.stringify(message));
        }
    }
    function sortModel() {
        var sortProperty = table.getColumn(table.sortIndicatorColumn).role;
        var before = (table.sortIndicatorOrder === Qt.AscendingOrder) ? -1 : 1;
        var after = -1 * before;
        userModelData.sort(function (a, b) {
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
        userModelData.forEach(function (datum) {
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
            userIds.push(userModelData[userIndex].sessionId);
        });
        pal.sendToScript({method: 'selected', params: userIds});
    }
    Connections {
        target: table.selection
        onSelectionChanged: pal.noticeSelection()
    }
}
