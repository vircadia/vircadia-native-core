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
import HFWebEngineProfile 1.0

// references HMD, Users, UserActivityLogger from root context

Rectangle {
    id: pal;
    // Size
    width: parent.width;
    height: parent.height;
    // Style
    color: "#E3E3E3";
    // Properties
    property int myCardWidth: palContainer.width - upperRightInfoContainer.width;
    property int myCardHeight: 82;
    property int rowHeight: 60;
    property int actionButtonWidth: 55;
    property int locationColumnWidth: 170;
    property int nearbyNameCardWidth: nearbyTable.width - (iAmAdmin ? (actionButtonWidth * 4) : (actionButtonWidth * 2)) - 4 - hifi.dimensions.scrollbarBackgroundWidth;
    property int connectionsNameCardWidth: connectionsTable.width - locationColumnWidth - actionButtonWidth - 4 - hifi.dimensions.scrollbarBackgroundWidth;
    property var myData: ({profileUrl: "", displayName: "", userName: "", audioLevel: 0.0, avgAudioLevel: 0.0, admin: true, placeName: "", connection: ""}); // valid dummy until set
    property var ignored: ({}); // Keep a local list of ignored avatars & their data. Necessary because HashMap is slow to respond after ignoring.
    property var nearbyUserModelData: []; // This simple list is essentially a mirror of the nearbyUserModel listModel without all the extra complexities.
    property var connectionsUserModelData: []; // This simple list is essentially a mirror of the connectionsUserModel listModel without all the extra complexities.
    property bool iAmAdmin: false;
    property var activeTab: "nearbyTab";
    property int usernameAvailability;
    property bool currentlyEditingDisplayName: false

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
        property int nearbySortIndicatorColumn: 1;
        property int nearbySortIndicatorOrder: Qt.AscendingOrder;
        property int connectionsSortIndicatorColumn: 0;
        property int connectionsSortIndicatorOrder: Qt.AscendingOrder;
    }
    function getSelectedNearbySessionIDs() {
        var sessionIDs = [];
        nearbyTable.selection.forEach(function (userIndex) {
            var datum = nearbyUserModelData[userIndex];
            if (datum) { // Might have been filtered out
                sessionIDs.push(datum.sessionId);
            }
        });
        return sessionIDs;
    }
    function getSelectedConnectionsUserNames() {
        var userNames = [];
        connectionsTable.selection.forEach(function (userIndex) {
            var datum = connectionsUserModelData[userIndex];
            if (datum) {
                userNames.push(datum.userName);
            }
        });
        return userNames;
    }
    function refreshNearbyWithFilter() {
        // We should just be able to set settings.filtered to inViewCheckbox.checked, but see #3249, so send to .js for saving.
        var userIds = getSelectedNearbySessionIDs();
        var params = {filter: inViewCheckbox.checked && {distance: settings.nearDistance}};
        if (userIds.length > 0) {
            params.selected = [[userIds[0]], true, true];
        }
        pal.sendToScript({method: 'refreshNearby', params: params});
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
            // This NameCard refers to the current user's NameCard (the one above the nearbyTable)
            NameCard {
                id: myCard;
                // Properties
                profileUrl: myData.profileUrl;
                imageMaskColor: pal.color;
                displayName: myData.displayName;
                userName: myData.userName;
                audioLevel: myData.audioLevel;
                avgAudioLevel: myData.avgAudioLevel;
                isMyCard: true;
                // Size
                width: myCardWidth;
                height: parent.height;
                // Anchors
                anchors.top: parent.top
                anchors.left: parent.left;
            }
            Item {
                id: upperRightInfoContainer;
                width: 160;
                height: parent.height;
                anchors.top: parent.top;
                anchors.right: parent.right;
                
                RalewayRegular {
                    id: availabilityText;
                    text: "set availability";
                    // Text size
                    size: hifi.fontSizes.tabularData;
                    // Anchors
                    anchors.top: availabilityComboBox.bottom;
                    anchors.horizontalCenter: parent.horizontalCenter;
                    // Style
                    color: hifi.colors.baseGrayHighlight;
                    // Alignment
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignTop;
                }
                HifiControls.TabletComboBox {
                    id: availabilityComboBox;
                    // Anchors
                    anchors.top: parent.top;
                    anchors.horizontalCenter: parent.horizontalCenter;
                    // Size
                    width: parent.width;
                    height: 40;
                    currentIndex: usernameAvailability;
                    model: ListModel {
                        id: availabilityComboBoxListItems
                        ListElement { text: "Everyone"; value: "all"; }
                        ListElement { text: "Friends Only"; value: "friends"; }
                        ListElement { text: "Appear Offline"; value: "none" }
                    }
                    onCurrentIndexChanged: { pal.sendToScript({method: 'setAvailability', params: availabilityComboBoxListItems.get(currentIndex).value})}
                }
            }
        }
    Item {
        id: palTabContainer;
        // Anchors
        anchors {
            top: myInfo.bottom;
            bottom: parent.bottom;
            left: parent.left;
            right: parent.right;
        }
        Rectangle {
            id: tabSelectorContainer;
            // Anchors
            anchors {
                top: parent.top;
                topMargin: 2;
                horizontalCenter: parent.horizontalCenter;
            }
            width: parent.width;
            height: 35 - anchors.topMargin;
            Rectangle {
                id: nearbyTabSelector;
                // Anchors
                anchors {
                    top: parent.top;
                    left: parent.left;
                }
                width: parent.width/2;
                height: parent.height;
                color: activeTab == "nearbyTab" ? pal.color : "#CCCCCC";
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        if (activeTab != "nearbyTab") {
                            refreshNearbyWithFilter();
                        }
                        activeTab = "nearbyTab";
                    }
                }

                // "NEARBY" Text Container
                Item {
                    id: nearbyTabSelectorTextContainer;
                    anchors.fill: parent;
                    anchors.leftMargin: 15;
                    // "NEARBY" text
                    RalewaySemiBold {
                        id: nearbyTabSelectorText;
                        text: "NEARBY";
                        // Text size
                        size: hifi.fontSizes.tabularData;
                        // Anchors
                        anchors.fill: parent;
                        // Style
                        font.capitalization: Font.AllUppercase;
                        color: hifi.colors.redHighlight;
                        // Alignment
                        horizontalAlignment: Text.AlignHLeft;
                        verticalAlignment: Text.AlignVCenter;
                    }
                    // "In View" Checkbox
                    HifiControls.CheckBox {
                        id: inViewCheckbox;
                        visible: activeTab == "nearbyTab";
                        anchors.right: reloadNearbyContainer.left;
                        anchors.rightMargin: 25;
                        anchors.verticalCenter: parent.verticalCenter;
                        checked: settings.filtered;
                        text: "in view";
                        boxSize: 24;
                        onCheckedChanged: refreshNearbyWithFilter();
                    }
                    // Refresh button
                    Rectangle {
                        id: reloadNearbyContainer
                        visible: activeTab == "nearbyTab";
                        anchors.verticalCenter: parent.verticalCenter;
                        anchors.right: parent.right;
                        anchors.rightMargin: 6;
                        height: reloadNearby.height;
                        width: height;
                        HifiControls.GlyphButton {
                            id: reloadNearby;
                            width: reloadNearby.height;
                            glyph: hifi.glyphs.reload;
                            onClicked: refreshNearbyWithFilter();
                        }
                    }
                }
            }
            Rectangle {
                id: connectionsTabSelector;
                // Anchors
                anchors {
                    top: parent.top;
                    left: nearbyTabSelector.right;
                }
                width: parent.width/2;
                height: parent.height;
                color: activeTab == "connectionsTab" ? pal.color : "#CCCCCC";
                MouseArea {
                    anchors.fill: parent;
                    onClicked: { 
                        if (activeTab != "connectionsTab") {
                            pal.sendToScript({method: 'refreshConnections'});
                        }
                        activeTab = "connectionsTab";
                        connectionsLoading.visible = false;
                        connectionsLoading.visible = true;
                        connectionsRefreshProblemText.visible = false;
                    }
                }

                // "CONNECTIONS" Text Container
                Item {
                    id: connectionsTabSelectorTextContainer;
                    anchors.fill: parent;
                    anchors.leftMargin: 15;
                    // Refresh button
                    Rectangle {
                        visible: activeTab == "connectionsTab";
                        anchors.verticalCenter: parent.verticalCenter;
                        anchors.right: parent.right;
                        anchors.rightMargin: 6;
                        height: reloadConnections.height;
                        width: height;
                        HifiControls.GlyphButton {
                            id: reloadConnections;
                            width: reloadConnections.height;
                            glyph: hifi.glyphs.reload;
                            onClicked: {
                                connectionsLoading.visible = false;
                                connectionsLoading.visible = true;
                                pal.sendToScript({method: 'refreshConnections'});
                            }
                        }
                    }
                    // "CONNECTIONS" text
                    RalewaySemiBold {
                        id: connectionsTabSelectorText;
                        text: "CONNECTIONS";
                        // Text size
                        size: hifi.fontSizes.tabularData;
                        // Anchors
                        anchors.fill: parent;
                        // Style
                        font.capitalization: Font.AllUppercase;
                        color: hifi.colors.redHighlight;
                        // Alignment
                        horizontalAlignment: Text.AlignHLeft;
                        verticalAlignment: Text.AlignVCenter;
                    }
                    TextMetrics {
                        id: connectionsTabSelectorTextMetrics;
                        text: connectionsTabSelectorText.text;
                    }

                    // This Rectangle refers to the [?] popup button next to "CONNECTIONS"
                    Rectangle {
                        color: connectionsTabSelector.color;
                        width: 20;
                        height: connectionsTabSelectorText.height - 2;
                        anchors.left: connectionsTabSelectorTextContainer.left;
                        anchors.top: connectionsTabSelectorTextContainer.top;
                        anchors.topMargin: 1;
                        anchors.leftMargin: connectionsTabSelectorTextMetrics.width + 42;
                        RalewayRegular {
                            id: connectionsHelpText;
                            text: "[?]";
                            size: connectionsTabSelectorText.size + 6;
                            font.capitalization: Font.AllUppercase;
                            color: hifi.colors.redHighlight;
                            horizontalAlignment: Text.AlignHCenter;
                            verticalAlignment: Text.AlignVCenter;
                            anchors.fill: parent;
                        }
                        MouseArea {
                            anchors.fill: parent;
                            hoverEnabled: true;
                            onClicked: letterbox(hifi.glyphs.question,
                                                 "Connections and Friends",
                                                 "<font color='purple'>Purple borders around profile pictures are <b>Connections</b>.</font><br>" +
                                                 "When your availability is set to Everyone, Connections can see your username and location.<br><br>" +
                                                 "<font color='green'>Green borders around profile pictures are <b>Friends</b>.</font><br>" +
                                                 "When your availability is set to Friends, only Friends can see your username and location.");
                            onEntered: connectionsHelpText.color = hifi.colors.redAccent;
                            onExited: connectionsHelpText.color = hifi.colors.redHighlight;
                        }
                    }
                }
            }
        }
        Item {
            id: tabBorders;
            anchors.fill: parent;
            property var color: hifi.colors.lightGray;
            property int borderWeight: 3;
            // Left border
            Rectangle {
                color: parent.color;
                anchors {
                    left: parent.left;
                    bottom: parent.bottom;
                }
                width: parent.borderWeight;
                height: parent.height - (activeTab == "nearbyTab" ? 0 : tabSelectorContainer.height);
            }
            // Right border
            Rectangle {
                color: parent.color;
                anchors {
                    right: parent.right;
                    bottom: parent.bottom;
                }
                width: parent.borderWeight;
                height: parent.height - (activeTab == "nearbyTab" ? tabSelectorContainer.height : 0);
            }
            // Bottom border
            Rectangle {
                color: parent.color;
                anchors {
                    bottom: parent.bottom;
                    left: parent.left;
                    right: parent.right;
                }
                height: parent.borderWeight;
            }
            // Border between buttons
            Rectangle {
                color: parent.color;
                anchors {
                    horizontalCenter: parent.horizontalCenter;
                    top: parent.top;
                }
                width: parent.borderWeight;
                height: tabSelectorContainer.height + width;
            }
            // Border above selected tab
            Rectangle {
                color: parent.color;
                anchors {
                    top: parent.top;
                    left: parent.left;
                    leftMargin: activeTab == "nearbyTab" ? 0 : parent.width/2;
                }
                width: parent.width/2;
                height: parent.borderWeight;
            }
            // Border below unselected tab
            Rectangle {
                color: parent.color;
                anchors {
                    top: parent.top;
                    topMargin: tabSelectorContainer.height;
                    left: parent.left;
                    leftMargin: activeTab == "nearbyTab" ? parent.width/2 : 0;
                }
                width: parent.width/2;
                height: parent.borderWeight;
            }
        }
        
    /*****************************************
                   NEARBY TAB
    *****************************************/
    Rectangle {
        id: nearbyTab;
        // Anchors
        anchors {
            top: tabSelectorContainer.bottom;
            topMargin: 12 + (iAmAdmin ? -adminTab.anchors.topMargin : 0);
            bottom: parent.bottom;
            bottomMargin: 12;
            horizontalCenter: parent.horizontalCenter;
        }
        width: parent.width - 12;
        visible: activeTab == "nearbyTab";

        // Rectangle that houses "ADMIN" string
        Rectangle {
            id: adminTab;
            // Size
            width: 2*actionButtonWidth + hifi.dimensions.scrollbarBackgroundWidth + 6;
            height: 40;
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: -30;
            anchors.right: parent.right;
            // Properties
            visible: iAmAdmin;
            // Style
            color: hifi.colors.tableRowLightEven;
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
        // This TableView refers to the Nearby Table (on the "Nearby" tab below the current user's NameCard)
        HifiControls.Table {
            id: nearbyTable;
            // Anchors
            anchors.fill: parent;
            // Properties
            centerHeaderText: true;
            sortIndicatorVisible: true;
            headerVisible: true;
            sortIndicatorColumn: settings.nearbySortIndicatorColumn;
            sortIndicatorOrder: settings.nearbySortIndicatorOrder;
            onSortIndicatorColumnChanged: {
                settings.nearbySortIndicatorColumn = sortIndicatorColumn;
                sortModel();
            }
            onSortIndicatorOrderChanged: {
                settings.nearbySortIndicatorOrder = sortIndicatorOrder;
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
                title: nearbyTable.rowCount + (nearbyTable.rowCount === 1 ? " NAME" : " NAMES");
                width: nearbyNameCardWidth;
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
                id: nearbyUserModel;
            }

            // This Rectangle refers to each Row in the nearbyTable.
            rowDelegate: Rectangle { // The only way I know to specify a row height.
                // Size
                height: styleData.selected ? rowHeight + 15 : rowHeight;
                color: rowColor(styleData.selected, styleData.alternate);
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
                    profileUrl: (model && model.profileUrl) || "";
                    imageMaskColor: rowColor(styleData.selected, styleData.row % 2);
                    displayName: styleData.value;
                    userName: model ? model.userName : "";
                    connectionStatus: model ? model.connection : "";
                    audioLevel: model ? model.audioLevel : 0.0;
                    avgAudioLevel: model ? model.avgAudioLevel : 0.0;
                    visible: !isCheckBox && !isButton && !isAvgAudio;
                    uuid: model ? model.sessionId : "";
                    selected: styleData.selected;
                    isAdmin: model && model.admin;
                    // Size
                    width: nearbyNameCardWidth;
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
                            nearbyUserModel.setProperty(model.userIndex, "personalMute", newValue);
                            nearbyUserModelData[model.userIndex]["personalMute"] = newValue; // Defensive programming
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
                        nearbyUserModel.setProperty(model.userIndex, styleData.role, newValue);
                        nearbyUserModelData[model.userIndex][styleData.role] = newValue; // Defensive programming
                        Users[styleData.role](model.sessionId, newValue);
                        UserActivityLogger["palAction"](newValue ? styleData.role : "un-" + styleData.role, model.sessionId);
                        if (styleData.role === "ignore") {
                            nearbyUserModel.setProperty(model.userIndex, "personalMute", newValue);
                            nearbyUserModelData[model.userIndex]["personalMute"] = newValue; // Defensive programming
                            if (newValue) {
                                ignored[model.sessionId] = nearbyUserModelData[model.userIndex];
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
                            nearbyUserModelData.splice(model.userIndex, 1);
                            nearbyUserModel.remove(model.userIndex); // after changing nearbyUserModelData, b/c ListModel can frob the data
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
            height: nearbyTable.height;
            // Anchors
            anchors.left: adminTab.left;
            anchors.top: nearbyTable.top;
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
            anchors.left: nearbyTable.left;
            anchors.top: nearbyTable.top;
            anchors.topMargin: 1;
            anchors.leftMargin: actionButtonWidth + nearbyNameCardWidth/2 + displayNameHeaderMetrics.width/2 + 6;
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
                hoverEnabled: true;
                onClicked: letterbox(hifi.glyphs.question,
                                     "Display Names",
                                     "Bold names in the list are <b>avatar display names</b>.<br>" +
                                     "<font color='purple'>Purple borders around profile pictures are <b>connections</b></font>.<br>" +
                                     "<font color='green'>Green borders around profile pictures are <b>friends</b>.</font><br>" +
                                     "(TEMPORARY LANGUAGE) In some situations, you can also see others' usernames.<br>" +
                                     "If you can see someone's username, you can GoTo them by selecting them in the PAL, then clicking their name.<br>" +
                                     "<br>If someone's display name isn't set, a unique <b>session display name</b> is assigned to them.<br>" +
                                     "<br>Administrators of this domain can also see the <b>username</b> or <b>machine ID</b> associated with each avatar present.");
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
            anchors.rightMargin: 12 + hifi.dimensions.scrollbarBackgroundWidth;
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
                hoverEnabled: true;
                onClicked: letterbox(hifi.glyphs.question,
                                     "Admin Actions",
                                     "<b>Silence</b> mutes a user's microphone. Silenced users can unmute themselves by clicking &quot;UNMUTE&quot; on their toolbar.<br><br>" +
                                     "<b>Ban</b> removes a user from this domain and prevents them from returning. Admins can un-ban users from the Sandbox Domain Settings page.");
                onEntered: adminHelpText.color = "#94132e";
                onExited: adminHelpText.color = hifi.colors.redHighlight;
            }
        }
    } // "Nearby" Tab


    /*****************************************
                CONNECTIONS TAB
    *****************************************/
    Rectangle {
        id: connectionsTab;
        color: "#E3E3E3";
        // Anchors
        anchors {
            top: tabSelectorContainer.bottom;
            topMargin: 12;
            bottom: parent.bottom;
            bottomMargin: 12;
            horizontalCenter: parent.horizontalCenter;
        }
        width: parent.width - 12;
        visible: activeTab == "connectionsTab";
        
        AnimatedImage {
            id: connectionsLoading;
            source: "../../icons/profilePicLoading.gif"
            width: 120;
            height: width;
            anchors.centerIn: parent;
            visible: true;
            onVisibleChanged: {
                if (visible) {
                    connectionsTimeoutTimer.start();
                } else {
                    connectionsTimeoutTimer.stop();     
                    connectionsRefreshProblemText.visible = false;               
                }
            }
        }

        // "This is taking too long..." text
        FiraSansSemiBold {
            id: connectionsRefreshProblemText
            // Properties
            text: "This is taking longer than normal.\nIf you get stuck, try refreshing the Connections tab.";
            // Anchors
            anchors.top: connectionsLoading.bottom;
            anchors.topMargin: 10;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width;
            // Text Size
            size: 16;
            // Text Positioning
            verticalAlignment: Text.AlignTop;
            horizontalAlignment: Text.AlignHCenter;
            wrapMode: Text.WordWrap;
            // Style
            color: hifi.colors.darkGray;
        }

        // This TableView refers to the Connections Table (on the "Connections" tab below the current user's NameCard)
        HifiControls.Table {
            id: connectionsTable;
            visible: !connectionsLoading.visible;
            // Anchors
            anchors.fill: parent;
            // Properties
            centerHeaderText: true;
            sortIndicatorVisible: true;
            headerVisible: true;
            sortIndicatorColumn: settings.connectionsSortIndicatorColumn;
            sortIndicatorOrder: settings.connectionsSortIndicatorOrder;
            onSortIndicatorColumnChanged: {
                settings.connectionsSortIndicatorColumn = sortIndicatorColumn;
                sortConnectionsModel();
            }
            onSortIndicatorOrderChanged: {
                settings.connectionsSortIndicatorOrder = sortIndicatorOrder;
                sortConnectionsModel();
            }

            TableViewColumn {
                id: connectionsUserNameHeader;
                role: "userName";
                title: connectionsTable.rowCount + (connectionsTable.rowCount === 1 ? " NAME" : " NAMES");
                width: connectionsNameCardWidth;
                movable: false;
                resizable: false;
            }
            TableViewColumn {
                role: "placeName";
                title: "LOCATION";
                width: locationColumnWidth;
                movable: false;
                resizable: false;
            }
            TableViewColumn {
                role: "friends";
                title: "FRIEND";
                width: actionButtonWidth;
                movable: false;
                resizable: false;
            }

            model: ListModel {
                id: connectionsUserModel;
            }

            // This Rectangle refers to each Row in the connectionsTable.
            rowDelegate: Rectangle {
                // Size
                height: rowHeight;
                color: rowColor(styleData.selected, styleData.alternate);
            }

            // This Item refers to the contents of each Cell
            itemDelegate: Item {
                id: connectionsItemCell;

                // This NameCard refers to the cell that contains a connection's UserName
                NameCard {
                    id: connectionsNameCard;
                    // Properties
                    visible: styleData.role === "userName";
                    profileUrl: (model && model.profileUrl) || "";
                    imageMaskColor: rowColor(styleData.selected, styleData.row % 2);
                    displayName: "";
                    userName: model ? model.userName : "";
                    connectionStatus : model ? model.connection : "";
                    selected: styleData.selected;
                    // Size
                    width: connectionsNameCardWidth;
                    height: parent.height;
                    // Anchors
                    anchors.left: parent.left;
                }

                // LOCATION data
                FiraSansSemiBold {
                    id: connectionsLocationData
                    // Properties
                    visible: styleData.role === "placeName";
                    text: (model && model.placeName) || "";
                    elide: Text.ElideRight;
                    // Size
                    width: parent.width;
                    // Anchors
                    anchors.fill: parent;
                    // Text Size
                    size: 16;
                    // Text Positioning
                    verticalAlignment: Text.AlignVCenter
                    // Style
                    color: hifi.colors.darkGray;
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: enabled
                        enabled: connectionsNameCard.selected && pal.activeTab == "connectionsTab"
                        onClicked: pal.sendToScript({method: 'goToUser', params: model.userName});
                        onEntered: connectionsLocationData.color = hifi.colors.blueHighlight;
                        onExited: connectionsLocationData.color = hifi.colors.darkGray;
                    }
                }

                // "Friends" checkbox
                HifiControls.CheckBox {
                    id: friendsCheckBox;
                    visible: styleData.role === "friends";
                    anchors.centerIn: parent;
                    checked: model ? (model["connection"] === "friend" ? true : false) : false;
                    boxSize: 24;
                    onClicked: {
                        var newValue = !model[styleData.role];
                        connectionsUserModel.setProperty(model.userIndex, styleData.role, newValue);
                        connectionsUserModelData[model.userIndex][styleData.role] = newValue; // Defensive programming
                        // Insert line here about actually taking the friend/unfriend action
                        // Also insert line here about logging the activity, similar to the commented line below
                        //UserActivityLogger["palAction"](newValue ? styleData.role : "un-" + styleData.role, model.sessionId);

                        // http://doc.qt.io/qt-5/qtqml-syntax-propertybinding.html#creating-property-bindings-from-javascript
                        // I'm using an explicit binding here because clicking a checkbox breaks the implicit binding as set by
                        // "checked:" statement above.
                        checked = Qt.binding(function() { return (model["connection"] === "friend" ? true : false)});
                    }
                }
            }
        }
    } // "Connections" Tab
    } // palTabContainer

        HifiControls.Keyboard {
            id: keyboard;
            raised: currentlyEditingDisplayName && HMD.mounted;
            numeric: parent.punctuationMode;
            anchors {
                bottom: parent.bottom;
                left: parent.left;
                right: parent.right;
            }
        } // Keyboard

        Item {
            id: webViewContainer;
            anchors.fill: parent;

            Rectangle {
                id: navigationContainer;
                visible: userInfoViewer.visible;
                height: 60;
                anchors {
                    top: parent.top;
                    left: parent.left;
                    right: parent.right;
                }
                color: hifi.colors.faintGray;

                Item {
                    id: backButton
                    anchors {
                        top: parent.top;
                        left: parent.left;
                    }
                    height: parent.height - addressBar.height;
                    width: parent.width/2;

                    FiraSansSemiBold {
                        // Properties
                        text: "BACK";
                        elide: Text.ElideRight;
                        // Anchors
                        anchors.fill: parent;
                        // Text Size
                        size: 16;
                        // Text Positioning
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter;
                        // Style
                        color: backButtonMouseArea.containsMouse || !userInfoViewer.canGoBack ? hifi.colors.lightGray : hifi.colors.darkGray;
                        MouseArea {
                            id: backButtonMouseArea;
                            anchors.fill: parent
                            hoverEnabled: enabled
                            onClicked: {
                                if (userInfoViewer.canGoBack) {
                                    userInfoViewer.goBack();
                                }
                            }
                        }
                    }
                }

                Item {
                    id: closeButtonContainer
                    anchors {
                        top: parent.top;
                        right: parent.right;
                    }
                    height: parent.height - addressBar.height;
                    width: parent.width/2;

                    FiraSansSemiBold {
                        id: closeButton;
                        // Properties
                        text: "CLOSE";
                        elide: Text.ElideRight;
                        // Anchors
                        anchors.fill: parent;
                        // Text Size
                        size: 16;
                        // Text Positioning
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter;
                        // Style
                        color: hifi.colors.redHighlight;
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: enabled
                            onClicked: userInfoViewer.visible = false;
                            onEntered: closeButton.color = hifi.colors.redAccent;
                            onExited: closeButton.color = hifi.colors.redHighlight;
                        }
                    }
                }

                Item {
                    id: addressBar
                    anchors {
                        top: closeButtonContainer.bottom;
                        left: parent.left;
                        right: parent.right;
                    }
                    height: 30;
                    width: parent.width;

                    FiraSansRegular {
                        // Properties
                        text: userInfoViewer.url;
                        elide: Text.ElideRight;
                        // Anchors
                        anchors.fill: parent;
                        anchors.leftMargin: 5;
                        // Text Size
                        size: 14;
                        // Text Positioning
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft;
                        // Style
                        color: hifi.colors.lightGray;
                    }
                }
            }

            Rectangle {
                id: webViewBackground;
                color: "white";
                visible: userInfoViewer.visible;
                anchors {
                    top: navigationContainer.bottom;
                    bottom: parent.bottom;
                    left: parent.left;
                    right: parent.right;
                }
            }

            HifiControls.WebView {
                id: userInfoViewer;
                profile: HFWebEngineProfile {
                    storageName: "qmlWebEngine"
                }
                anchors {
                    top: navigationContainer.bottom;
                    bottom: parent.bottom;
                    left: parent.left;
                    right: parent.right;
                }
                visible: false;
            }
        }


    } // PAL container

    // Timer used when selecting nearbyTable rows that aren't yet present in the model
    // (i.e. when selecting avatars using edit.js or sphere overlays)
    Timer {
        property bool selected; // Selected or deselected?
        property int userIndex; // The userIndex of the avatar we want to select
        id: selectionTimer;
        onTriggered: {
            if (selected) {
                nearbyTable.selection.clear(); // for now, no multi-select
                nearbyTable.selection.select(userIndex);
                nearbyTable.positionViewAtRow(userIndex, ListView.Beginning);
            } else {
                nearbyTable.selection.deselect(userIndex);
            }
        }
    }

    // Timer used when refreshing the Connections tab
    Timer {
        id: connectionsTimeoutTimer;
        interval: 3000; // 3 seconds
        onTriggered: {
            connectionsRefreshProblemText.visible = true;
        }
    }

    function rowColor(selected, alternate) {
        return selected ? hifi.colors.orangeHighlight : alternate ? hifi.colors.tableRowLightEven : hifi.colors.tableRowLightOdd;
    }
    function findNearbySessionIndex(sessionId, optionalData) { // no findIndex in .qml
        var data = optionalData || nearbyUserModelData, length = data.length;
        for (var i = 0; i < length; i++) {
            if (data[i].sessionId === sessionId) {
                return i;
            }
        }
        return -1;
    }
    function fromScript(message) {
        switch (message.method) {
        case 'nearbyUsers':
            var data = message.params;
            var index = -1;
            index = findNearbySessionIndex('', data);
            if (index !== -1) {
                iAmAdmin = Users.canKick;
                myData = data[index];
                data.splice(index, 1);
            } else {
                console.log("This user's data was not found in the user list. PAL will not function properly.");
            }
            nearbyUserModelData = data;
            for (var ignoredID in ignored) {
                index = findNearbySessionIndex(ignoredID);
                if (index === -1) { // Add back any missing ignored to the PAL, because they sometimes take a moment to show up.
                    nearbyUserModelData.push(ignored[ignoredID]);
                } else { // Already appears in PAL; update properties of existing element in model data
                    nearbyUserModelData[index] = ignored[ignoredID];
                }
            }
            sortModel();
            break;
        case 'connections':
            var data = message.params;
            console.log('Got connection data: ', JSON.stringify(data));
            connectionsUserModelData = data;
            sortConnectionsModel();
            connectionsLoading.visible = false;
            connectionsRefreshProblemText.visible = false;
            break;
        case 'select':
            var sessionIds = message.params[0];
            var selected = message.params[1];
            var alreadyRefreshed = message.params[2];
            var userIndex = findNearbySessionIndex(sessionIds[0]);
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
                    // Wait a little bit before trying to actually select the avatar in the nearbyTable
                    selectionTimer.interval = 250;
                } else {
                    // If we've found the avatar in the model and didn't need to refresh,
                    // select the avatar in the nearbyTable immediately
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
            var userId = message.params.sessionId;
            // If the userId is empty, we're probably updating "myData".
            if (userId) {
                // Get the index in nearbyUserModel and nearbyUserModelData associated with the passed UUID
                var userIndex = findNearbySessionIndex(userId);
                if (userIndex !== -1) {
                    ['userName', 'admin', 'connection', 'profileUrl', 'placeName'].forEach(function (name) {
                        var value = message.params[name];
                        if (value === undefined) {
                            return;
                        }
                        nearbyUserModel.setProperty(userIndex, name, value);
                        nearbyUserModelData[userIndex][name] = value; // for refill after sort
                    });
                }
            // In this "else if" case, the only param of the message is the profile pic URL.
            } else if (message.params.profileUrl) {
                myData.profileUrl = message.params.profileUrl;
                myCard.profileUrl = message.params.profileUrl;
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
                    var userIndex = findNearbySessionIndex(userId);
                    if (userIndex != -1) {
                        nearbyUserModel.setProperty(userIndex, "audioLevel", audioLevel);
                        nearbyUserModelData[userIndex].audioLevel = audioLevel; // Defensive programming
                        nearbyUserModel.setProperty(userIndex, "avgAudioLevel", avgAudioLevel);
                        nearbyUserModelData[userIndex].avgAudioLevel = avgAudioLevel;
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
        case 'updateAvailability':
            usernameAvailability = message.params;
            break;
        default:
            console.log('Unrecognized message:', JSON.stringify(message));
        }
    }
    function sortModel() {
        var column = nearbyTable.getColumn(nearbyTable.sortIndicatorColumn);
        var sortProperty = column ? column.role : "displayName";
        var before = (nearbyTable.sortIndicatorOrder === Qt.AscendingOrder) ? -1 : 1;
        var after = -1 * before;
        // get selection(s) before sorting
        var selectedIDs = getSelectedNearbySessionIDs();
        nearbyUserModelData.sort(function (a, b) {
            var aValue = a[sortProperty].toString().toLowerCase(), bValue = b[sortProperty].toString().toLowerCase();
            switch (true) {
            case (aValue < bValue): return before;
            case (aValue > bValue): return after;
            default: return 0;
            }
        });
        nearbyTable.selection.clear();

        nearbyUserModel.clear();
        var userIndex = 0;
        var newSelectedIndexes = [];
        nearbyUserModelData.forEach(function (datum) {
            function init(property) {
                if (datum[property] === undefined) {
                    // These properties must have values of type 'string'.
                    if (property === 'userName' || property === 'profileUrl' || property === 'placeName' || property === 'connection') {
                        datum[property] = "";
                    // All other properties must have values of type 'bool'.
                    } else {
                        datum[property] = false;
                    }
                }
            }
            ['personalMute', 'ignore', 'mute', 'kick', 'admin', 'userName', 'profileUrl', 'placeName', 'connection'].forEach(init);
            datum.userIndex = userIndex++;
            nearbyUserModel.append(datum);
            if (selectedIDs.indexOf(datum.sessionId) != -1) {
                 newSelectedIndexes.push(datum.userIndex);
            }
        });
        if (newSelectedIndexes.length > 0) {
            nearbyTable.selection.select(newSelectedIndexes);
            nearbyTable.positionViewAtRow(newSelectedIndexes[0], ListView.Beginning);
        }
    }
    function sortConnectionsModel() {
        var column = connectionsTable.getColumn(connectionsTable.sortIndicatorColumn);
        var sortProperty = column ? column.role : "userName";
        var before = (connectionsTable.sortIndicatorOrder === Qt.AscendingOrder) ? -1 : 1;
        var after = -1 * before;
        // get selection(s) before sorting
        var selectedIDs = getSelectedConnectionsUserNames();
        connectionsUserModelData.sort(function (a, b) {
            var aValue = a[sortProperty].toString().toLowerCase(), bValue = b[sortProperty].toString().toLowerCase();
            switch (true) {
            case (aValue < bValue): return before;
            case (aValue > bValue): return after;
            default: return 0;
            }
        });
        connectionsTable.selection.clear();

        connectionsUserModel.clear();
        var userIndex = 0;
        var newSelectedIndexes = [];
        connectionsUserModelData.forEach(function (datum) {
            datum.userIndex = userIndex++;
            connectionsUserModel.append(datum);
            if (selectedIDs.indexOf(datum.sessionId) != -1) {
                 newSelectedIndexes.push(datum.userIndex);
            }
        });
        if (newSelectedIndexes.length > 0) {
            connectionsTable.selection.select(newSelectedIndexes);
            connectionsTable.positionViewAtRow(newSelectedIndexes[0], ListView.Beginning);
        }
    }
    signal sendToScript(var message);
    function noticeSelection() {
        var userIds = [];
        nearbyTable.selection.forEach(function (userIndex) {
            userIds.push(nearbyUserModelData[userIndex].sessionId);
        });
        pal.sendToScript({method: 'selected', params: userIds});
    }
    Connections {
        target: nearbyTable.selection;
        onSelectionChanged: pal.noticeSelection();
    }
}
