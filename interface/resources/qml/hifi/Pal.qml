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

Rectangle {
    id: pal;
    // Size
    width: parent.width;
    height: parent.height;
    // Properties
    property int myCardHeight: 75;
    property int rowHeight: 65;
    property int separatorColWidth: 30;
    property int actionButtonWidth: 50;
    property int nameCardWidth: width - separatorColWidth-  actionButtonWidth*(table.columnCount - 2); // "-2" for Name and Separator cols;

    // This contains the current user's NameCard and will contain other information in the future
    Item {
        id: myInfo;
        // Size
        width: pal.width;
        height: myCardHeight;
        // Anchors
        anchors.top: pal.top;
        anchors.left: pal.left;
        anchors.topMargin: 10;
        anchors.bottomMargin: anchors.topMargin;
        // This NameCard refers to the current user's NameCard (the one above the table)
        NameCard {
            id: myCard;
            // Properties
            displayName: myData.displayName;
            userName: myData.userName;
            // Size
            width: nameCardWidth;
            height: parent.height;
            // Anchors
            anchors.left: parent.left;
        }
    }
    // This TableView refers to the table (below the current user's NameCard)
    TableView {
        id: table;
        // Size
        height: pal.height - myInfo.height;
        width: pal.width;
        // Anchors
        anchors.top: myInfo.bottom;
        // Properties
        frameVisible: false;
        sortIndicatorVisible: true;
        onSortIndicatorColumnChanged: sortModel();
        onSortIndicatorOrderChanged: sortModel();

        TableViewColumn {
            role: "displayName";
            title: "Name";
            width: nameCardWidth
        }
        TableViewColumn {
            role: "ignore";
            title: "Ignore"
            width: actionButtonWidth
        }
        TableViewColumn {
            title: "";
            width: separatorColWidth
        }
        TableViewColumn {
            visible: iAmAdmin;
            role: "mute";
            title: "Mute";
            width: actionButtonWidth
        }
        TableViewColumn {
            visible: iAmAdmin;
            role: "kick";
            title: "Ban"
            width: actionButtonWidth
        }
        model: userModel;

        // This Rectangle refers to each Row in the table.
        rowDelegate: Rectangle { // The only way I know to specify a row height.
            // Size
            height: rowHeight;
            // The rest of this is cargo-culted to restore the default styling
            SystemPalette {
                id: myPalette;
                colorGroup: SystemPalette.Active
            }
            color: {
                var baseColor = styleData.alternate?myPalette.alternateBase:myPalette.base
                return styleData.selected?myPalette.highlight:baseColor
            }
        }

        // This Item refers to the contents of each Cell
        itemDelegate: Item {
            id: itemCell;
            property bool isCheckBox: typeof(styleData.value) === 'boolean';
            property bool isSeparator: styleData.value === '';
            // Anchors
            anchors.topMargin: 10;
            anchors.bottomMargin: anchors.topMargin;
            // This NameCard refers to the cell that contains an avatar's
            // DisplayName and UserName
            NameCard {
                id: nameCard;
                // Properties
                displayName: styleData.value;
                userName: model.userName;
                visible: !isCheckBox && !isSeparator;
                // Size
                width: nameCardWidth;
                height: parent.height;
                // Anchors
                anchors.left: parent.left;
            }
            // This Rectangle refers to the cells that contain the action buttons
            Rectangle {
                radius: itemCell.height / 4;
                visible: isCheckBox && !isSeparator;
                color: styleData.value ? "green" : "red";
                anchors.fill: parent;
                MouseArea {
                    anchors.fill: parent;
                    acceptedButtons: Qt.LeftButton;
                    hoverEnabled: true;
                    onClicked: {
                        var newValue = !model[styleData.role];
                        var datum = userData[model.userIndex];
                        datum[styleData.role] = model[styleData.role] = newValue;
                        Users[styleData.role](model.sessionId);
                        // Just for now, while we cannot undo things:
                        userData.splice(model.userIndex, 1);
                        sortModel();
                    }
                }
            }
        }
    }

    property var userData: [];
    property var myData: ({displayName: "", userName: ""}); // valid dummy until set
    property bool iAmAdmin: false;
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
            ['ignore', 'spacer', 'mute', 'kick'].forEach(init);
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
