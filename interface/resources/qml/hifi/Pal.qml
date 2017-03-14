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
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0
import "../styles-uit"
import "../controls-uit" as HifiControls

// references HMD, Users, UserActivityLogger from root context

Rectangle {
    id: pal;
    // Size
    width: parent.width;
    height: parent.height;
    // Style
    color: "#E3E3E3";
    // Properties
    property int myCardHeight: 90;
    property int rowHeight: 70;
    property int actionButtonWidth: 55;
    property int actionButtonAllowance: actionButtonWidth * 2;
    property int minNameCardWidth: palContainer.width - (actionButtonAllowance * 2) - 4 - hifi.dimensions.scrollbarBackgroundWidth;
    property int nameCardWidth: minNameCardWidth + (iAmAdmin ? 0 : actionButtonAllowance);
    property var myData: ({displayName: "", userName: "", audioLevel: 0.0, avgAudioLevel: 0.0, admin: true}); // valid dummy until set
    property var ignored: ({}); // Keep a local list of ignored avatars & their data. Necessary because HashMap is slow to respond after ignoring.
    property var userModelData: []; // This simple list is essentially a mirror of the userModel listModel without all the extra complexities.
    property bool iAmAdmin: false;

    HifiConstants { id: hifi; }

    // The letterbox used for popup messages
    LetterboxMessage {

        id: letterboxMessage;
        z: 999; // Force the popup on top of everything else
    }
    function letterbox(headerGlyph, headerText, message) {
        letterboxMessage.headerGlyph = headerGlyph;
        letterboxMessage.headerText = headerText;
        letterboxMessage.text = message;
        letterboxMessage.visible = true;
        letterboxMessage.popupRadius = 0;
    }
    Settings {
        id: settings;
        category: "pal";
        property bool filtered: false;
        property int nearDistance: 30;
        property int sortIndicatorColumn: 1;
        property int sortIndicatorOrder: Qt.AscendingOrder;
    }
    function getSelectedSessionIDs() {
        var sessionIDs = [];
        table.selection.forEach(function (userIndex) {
            sessionIDs.push(userModelData[userIndex].sessionId);
        });
        return sessionIDs;
    }
    function refreshWithFilter() {
        // We should just be able to set settings.filtered to filter.checked, but see #3249, so send to .js for saving.
        var userIds = getSelectedSessionIDs();
        var params = {filter: filter.checked && {distance: settings.nearDistance}};
        if (userIds.length > 0) {
            params.selected = [[userIds[0]], true, true];
        }
        pal.sendToScript({method: 'refresh', params: params});
    }

    // This is the container for the PAL
    Rectangle {
        property bool punctuationMode: false;
        id: palContainer;
        // Size
        width: pal.width - 10;
        height: pal.height - 10;
        // Style
        color: pal.color;
        // Anchors
        anchors.centerIn: pal;
        // Properties
        radius: hifi.dimensions.borderRadius;

        // This contains the current user's NameCard and will contain other information in the future
        Rectangle {
            id: myInfo;
            // Size
            width: palContainer.width;
            height: myCardHeight;
            // Style
            color: pal.color;
            // Anchors
            anchors.top: palContainer.top;
            // Properties
            radius: hifi.dimensions.borderRadius;
            // This NameCard refers to the current user's NameCard (the one above the table)
            NameCard {
                id: myCard;
                // Properties
                displayName: myData.displayName;
                userName: myData.userName;
                audioLevel: myData.audioLevel;
                avgAudioLevel: myData.avgAudioLevel;
                isMyCard: true;
                // Size
                width: minNameCardWidth;
                height: parent.height;
                // Anchors
                anchors.left: parent.left;
            }
            Row {
                HifiControls.CheckBox {
                    id: filter;
                    checked: settings.filtered;
                    text: "in view";
                    boxSize: reload.height * 0.70;
                    onCheckedChanged: refreshWithFilter();
                }
                HifiControls.GlyphButton {
                    id: reload;
                    glyph: hifi.glyphs.reload;
                    width: reload.height;
                    onClicked: refreshWithFilter();
                }
                spacing: 50;
                anchors {
                    right: parent.right;
                    top: parent.top;
                    topMargin: 10;
                }
            }
        }
        // Rectangles used to cover up rounded edges on bottom of MyInfo Rectangle
        Rectangle {
            color: pal.color;
            width: palContainer.width;
            height: 10;
            anchors.top: myInfo.bottom;
            anchors.left: parent.left;
        }
        Rectangle {
            color: pal.color;
            width: palContainer.width;
            height: 10;
            anchors.bottom: table.top;
            anchors.left: parent.left;
        }
        // Rectangle that houses "ADMIN" string
        Rectangle {
            id: adminTab;
            // Size
            width: 2*actionButtonWidth + hifi.dimensions.scrollbarBackgroundWidth + 2;
            height: 40;
            // Anchors
            anchors.bottom: myInfo.bottom;
            anchors.bottomMargin: -10;
            anchors.right: myInfo.right;
            // Properties
            visible: iAmAdmin;
            // Style
            color: hifi.colors.tableRowLightEven;
            radius: hifi.dimensions.borderRadius;
            border.color: hifi.colors.lightGrayText;
            border.width: 2;
            // "ADMIN" text
            RalewaySemiBold {
                id: adminTabText;
                text: "ADMIN";
                // Text size
                size: hifi.fontSizes.tableHeading + 2;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 8;
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.rightMargin: hifi.dimensions.scrollbarBackgroundWidth;
                // Style
                font.capitalization: Font.AllUppercase;
                color: hifi.colors.redHighlight;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignTop;
            }
        }
        // This TableView refers to the table (below the current user's NameCard)
        HifiControls.Table {
            id: table;
            // Size
            height: palContainer.height - myInfo.height - 4;
            width: palContainer.width - 4;
            // Anchors
            anchors.left: parent.left;
            anchors.top: myInfo.bottom;
            // Properties
            centerHeaderText: true;
            sortIndicatorVisible: true;
            headerVisible: true;
            sortIndicatorColumn: settings.sortIndicatorColumn;
            sortIndicatorOrder: settings.sortIndicatorOrder;
            onSortIndicatorColumnChanged: {
                settings.sortIndicatorColumn = sortIndicatorColumn;
                sortModel();
            }
            onSortIndicatorOrderChanged: {
                settings.sortIndicatorOrder = sortIndicatorOrder;
                sortModel();
            }

            TableViewColumn {
                role: "avgAudioLevel";
                title: "LOUD";
                width: actionButtonWidth;
                movable: false;
                resizable: false;
            }

            TableViewColumn {
                id: displayNameHeader;
                role: "displayName";
                title: table.rowCount + (table.rowCount === 1 ? " NAME" : " NAMES");
                width: nameCardWidth;
                movable: false;
                resizable: false;
            }
            TableViewColumn {
                role: "ignore";
                title: "IGNORE";
                width: actionButtonWidth;
                movable: false;
                resizable: false;
            }
            TableViewColumn {
                visible: iAmAdmin;
                role: "mute";
                title: "SILENCE";
                width: actionButtonWidth;
                movable: false;
                resizable: false;
            }
            TableViewColumn {
                visible: iAmAdmin;
                role: "kick";
                title: "BAN";
                width: actionButtonWidth;
                movable: false;
                resizable: false;
            }
            model: ListModel {
                id: userModel;
            }

            // This Rectangle refers to each Row in the table.
            rowDelegate: Rectangle { // The only way I know to specify a row height.
                // Size
                height: styleData.selected ? rowHeight : rowHeight - 15;
                color: styleData.selected
                    ? hifi.colors.orangeHighlight
                    : styleData.alternate ? hifi.colors.tableRowLightEven : hifi.colors.tableRowLightOdd;
            }

            // This Item refers to the contents of each Cell
            itemDelegate: Item {
                id: itemCell;
                property bool isCheckBox: styleData.role === "personalMute" || styleData.role === "ignore";
                property bool isButton: styleData.role === "mute" || styleData.role === "kick";
                property bool isAvgAudio: styleData.role === "avgAudioLevel";

                // This NameCard refers to the cell that contains an avatar's
                // DisplayName and UserName
                NameCard {
                    id: nameCard;
                    // Properties
                    displayName: styleData.value;
                    userName: model ? model.userName : "";
                    audioLevel: model ? model.audioLevel : 0.0;
                    avgAudioLevel: model ? model.avgAudioLevel : 0.0;
                    visible: !isCheckBox && !isButton && !isAvgAudio;
                    uuid: model ? model.sessionId : "";
                    selected: styleData.selected;
                    isAdmin: model && model.admin;
                    // Size
                    width: nameCardWidth;
                    height: parent.height;
                    // Anchors
                    anchors.left: parent.left;
                }
                HifiControls.GlyphButton {
                    function getGlyph() {
                        var fileName = "vol_";
                        if (model && model.personalMute) {
                            fileName += "x_";
                        }
                        fileName += (4.0*(model ? model.avgAudioLevel : 0.0)).toFixed(0);
                        return hifi.glyphs[fileName];
                    }
                    id: avgAudioVolume;
                    visible: isAvgAudio;
                    glyph: getGlyph();
                    width: 32;
                    size: height;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.horizontalCenter: parent.horizontalCenter;
                    onClicked: {
                        // cannot change mute status when ignoring
                        if (!model["ignore"]) {
                            var newValue = !model["personalMute"];
                            userModel.setProperty(model.userIndex, "personalMute", newValue);
                            userModelData[model.userIndex]["personalMute"] = newValue; // Defensive programming
                            Users["personalMute"](model.sessionId, newValue);
                            UserActivityLogger["palAction"](newValue ? "personalMute" : "un-personalMute", model.sessionId);
                        }
                    }
                }

                // This CheckBox belongs in the columns that contain the stateful action buttons ("Mute" & "Ignore" for now)
                // KNOWN BUG with the Checkboxes: When clicking in the center of the sorting header, the checkbox
                // will appear in the "hovered" state. Hovering over the checkbox will fix it.
                // Clicking on the sides of the sorting header doesn't cause this problem.
                // I'm guessing this is a QT bug and not anything I can fix. I spent too long trying to work around it...
                // I'm just going to leave the minor visual bug in.
                HifiControls.CheckBox {
                    id: actionCheckBox;
                    visible: isCheckBox;
                    anchors.centerIn: parent;
                    checked: model ? model[styleData.role] : false;
                    // If this is a "Personal Mute" checkbox, disable the checkbox if the "Ignore" checkbox is checked.
                    enabled: !(styleData.role === "personalMute" && (model ? model["ignore"] : true));
                    boxSize: 24;
                    onClicked: {
                        var newValue = !model[styleData.role];
                        userModel.setProperty(model.userIndex, styleData.role, newValue);
                        userModelData[model.userIndex][styleData.role] = newValue; // Defensive programming
                        Users[styleData.role](model.sessionId, newValue);
                        UserActivityLogger["palAction"](newValue ? styleData.role : "un-" + styleData.role, model.sessionId);
                        if (styleData.role === "ignore") {
                            userModel.setProperty(model.userIndex, "personalMute", newValue);
                            userModelData[model.userIndex]["personalMute"] = newValue; // Defensive programming
                            if (newValue) {
                                ignored[model.sessionId] = userModelData[model.userIndex];
                            } else {
                                delete ignored[model.sessionId];
                            }
                            avgAudioVolume.glyph = avgAudioVolume.getGlyph();
                        }
                        // http://doc.qt.io/qt-5/qtqml-syntax-propertybinding.html#creating-property-bindings-from-javascript
                        // I'm using an explicit binding here because clicking a checkbox breaks the implicit binding as set by
                        // "checked:" statement above.
                        checked = Qt.binding(function() { return (model[styleData.role])});
                    }
                }

                // This Button belongs in the columns that contain the stateless action buttons ("Silence" & "Ban" for now)
                HifiControls.Button {
                    id: actionButton;
                    color: 2; // Red
                    visible: isButton;
                    anchors.centerIn: parent;
                    width: 32;
                    height: 32;
                    onClicked: {
                        Users[styleData.role](model.sessionId);
                        UserActivityLogger["palAction"](styleData.role, model.sessionId);
                        if (styleData.role === "kick") {
                            userModelData.splice(model.userIndex, 1);
                            userModel.remove(model.userIndex); // after changing userModelData, b/c ListModel can frob the data
                        }
                    }
                    // muted/error glyphs
                    HiFiGlyphs {
                        text: (styleData.role === "kick") ? hifi.glyphs.error : hifi.glyphs.muted;
                        // Size
                        size: parent.height*1.3;
                        // Anchors
                        anchors.fill: parent;
                        // Style
                        horizontalAlignment: Text.AlignHCenter;
                        color: enabled ? hifi.buttons.textColor[actionButton.color]
                            : hifi.buttons.disabledTextColor[actionButton.colorScheme];
                    }
                }
            }
        }

        // Separator between user and admin functions
        Rectangle {
            // Size
            width: 2;
            height: table.height;
            // Anchors
            anchors.left: adminTab.left;
            anchors.top: table.top;
            // Properties
            visible: iAmAdmin;
            color: hifi.colors.lightGrayText;
        }
        TextMetrics {
            id: displayNameHeaderMetrics;
            text: displayNameHeader.title;
            // font: displayNameHeader.font // was this always undefined? giving error now...
        }
        // This Rectangle refers to the [?] popup button next to "NAMES"
        Rectangle {
            color: hifi.colors.tableBackgroundLight;
            width: 20;
            height: hifi.dimensions.tableHeaderHeight - 2;
            anchors.left: table.left;
            anchors.top: table.top;
            anchors.topMargin: 1;
            anchors.leftMargin: actionButtonWidth + nameCardWidth/2 + displayNameHeaderMetrics.width/2 + 6;
            RalewayRegular {
                id: helpText;
                text: "[?]";
                size: hifi.fontSizes.tableHeading + 2;
                font.capitalization: Font.AllUppercase;
                color: hifi.colors.darkGray;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
                anchors.fill: parent;
            }
            MouseArea {
                anchors.fill: parent;
                acceptedButtons: Qt.LeftButton;
                hoverEnabled: true;
                onClicked: letterbox(hifi.glyphs.question,
                                     "Display Names",
                                     "Bold names in the list are <b>avatar display names</b>.<br>" +
                                     "If a display name isn't set, a unique <b>session display name</b> is assigned." +
                                     "<br><br>Administrators of this domain can also see the <b>username</b> or <b>machine ID</b> associated with each avatar present.");
                onEntered: helpText.color = hifi.colors.baseGrayHighlight;
                onExited: helpText.color = hifi.colors.darkGray;
            }
        }
        // This Rectangle refers to the [?] popup button next to "ADMIN"
        Rectangle {
            visible: iAmAdmin;
            color: adminTab.color;
            width: 20;
            height: 28;
            anchors.right: adminTab.right;
            anchors.rightMargin: 10 + hifi.dimensions.scrollbarBackgroundWidth;
            anchors.top: adminTab.top;
            anchors.topMargin: 2;
            RalewayRegular {
                id: adminHelpText;
                text: "[?]";
                size: hifi.fontSizes.tableHeading + 2;
                font.capitalization: Font.AllUppercase;
                color: hifi.colors.redHighlight;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
                anchors.fill: parent;
            }
            MouseArea {
                anchors.fill: parent;
                acceptedButtons: Qt.LeftButton;
                hoverEnabled: true;
                onClicked: letterbox(hifi.glyphs.question,
                                     "Admin Actions",
                                     "<b>Silence</b> mutes a user's microphone. Silenced users can unmute themselves by clicking &quot;UNMUTE&quot; on their toolbar.<br><br>" +
                                     "<b>Ban</b> removes a user from this domain and prevents them from returning. Admins can un-ban users from the Sandbox Domain Settings page.");
                onEntered: adminHelpText.color = "#94132e";
                onExited: adminHelpText.color = hifi.colors.redHighlight;
            }
        }

        HifiControls.Keyboard {
            id: keyboard;
            raised: myCard.currentlyEditingDisplayName && HMD.active;
            numeric: parent.punctuationMode;
            anchors {
                bottom: parent.bottom;
                left: parent.left;
                right: parent.right;
            }
        }
    }

    // Timer used when selecting table rows that aren't yet present in the model
    // (i.e. when selecting avatars using edit.js or sphere overlays)
    Timer {
        property bool selected; // Selected or deselected?
        property int userIndex; // The userIndex of the avatar we want to select
        id: selectionTimer;
        onTriggered: {
            if (selected) {
                table.selection.clear(); // for now, no multi-select
                table.selection.select(userIndex);
                table.positionViewAtRow(userIndex, ListView.Beginning);
            } else {
                table.selection.deselect(userIndex);
            }
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
            var alreadyRefreshed = message.params[2];
            var userIndex = findSessionIndex(sessionIds[0]);
            if (sessionIds.length > 1) {
                letterbox("", "", 'Only one user can be selected at a time.');
            } else if (userIndex < 0) {
                // If we've already refreshed the PAL and the avatar still isn't present in the model...
                if (alreadyRefreshed === true) {
                    letterbox('', '', 'The last editor of this object is either you or not among this list of users.');
                } else {
                    pal.sendToScript({method: 'refresh', params: {selected: message.params}});
                }
            } else {
                // If we've already refreshed the PAL and found the avatar in the model
                if (alreadyRefreshed === true) {
                    // Wait a little bit before trying to actually select the avatar in the table
                    selectionTimer.interval = 250;
                } else {
                    // If we've found the avatar in the model and didn't need to refresh,
                    // select the avatar in the table immediately
                    selectionTimer.interval = 0;
                }
                selectionTimer.selected = selected;
                selectionTimer.userIndex = userIndex;
                selectionTimer.start();
            }
            break;
        // Received an "updateUsername()" request from the JS
        case 'updateUsername':
            // The User ID (UUID) is the first parameter in the message.
            var userId = message.params[0];
            // The text that goes in the userName field is the second parameter in the message.
            var userName = message.params[1];
            var admin = message.params[2];
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
                    // Set the admin status appropriately
                    userModel.setProperty(userIndex, "admin", admin);
                    userModelData[userIndex].admin = admin; // Defensive programming
                }
            }
            break;
        case 'updateAudioLevel':
            for (var userId in message.params) {
                var audioLevel = message.params[userId][0];
                var avgAudioLevel = message.params[userId][1];
                // If the userId is 0, we're updating "myData".
                if (userId == 0) {
                    myData.audioLevel = audioLevel;
                    myCard.audioLevel = audioLevel; // Defensive programming
                    myData.avgAudioLevel = avgAudioLevel;
                    myCard.avgAudioLevel = avgAudioLevel;
                } else {
                    var userIndex = findSessionIndex(userId);
                    if (userIndex != -1) {
                        userModel.setProperty(userIndex, "audioLevel", audioLevel);
                        userModelData[userIndex].audioLevel = audioLevel; // Defensive programming
                        userModel.setProperty(userIndex, "avgAudioLevel", avgAudioLevel);
                        userModelData[userIndex].avgAudioLevel = avgAudioLevel;
                    }
                }
            }
            break;
        case 'clearLocalQMLData':
            ignored = {};
            break;
        case 'avatarDisconnected':
            var sessionID = message.params[0];
            delete ignored[sessionID];
            break;
        default:
            console.log('Unrecognized message:', JSON.stringify(message));
        }
    }
    function sortModel() {
        var column = table.getColumn(table.sortIndicatorColumn);
        var sortProperty = column ? column.role : "displayName";
        var before = (table.sortIndicatorOrder === Qt.AscendingOrder) ? -1 : 1;
        var after = -1 * before;
        // get selection(s) before sorting
        var selectedIDs = getSelectedSessionIDs();
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
        var newSelectedIndexes = [];
        userModelData.forEach(function (datum) {
            function init(property) {
                if (datum[property] === undefined) {
                    datum[property] = false;
                }
            }
            ['personalMute', 'ignore', 'mute', 'kick'].forEach(init);
            datum.userIndex = userIndex++;
            userModel.append(datum);
            if (selectedIDs.indexOf(datum.sessionId) != -1) {
                 newSelectedIndexes.push(datum.userIndex);
            }
        });
        if (newSelectedIndexes.length > 0) {
            table.selection.select(newSelectedIndexes);
            table.positionViewAtRow(newSelectedIndexes[0], ListView.Beginning);
        }
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
        target: table.selection;
        onSelectionChanged: pal.noticeSelection();
    }
}
