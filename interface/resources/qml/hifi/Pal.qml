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
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../controls" as HifiControls
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.

// references HMD, Users, UserActivityLogger from root context

Rectangle {
    id: pal;
    // Style
    color: "#E3E3E3";
    // Properties
    property bool debug: false;
    property int myCardWidth: width - upperRightInfoContainer.width;
    property int myCardHeight: 100;
    property int rowHeight: 60;
    property int actionButtonWidth: 65;
    property int locationColumnWidth: 170;
    property int nearbyNameCardWidth: nearbyTable.width - (iAmAdmin ? (actionButtonWidth * 4) : (actionButtonWidth * 2)) - 4 - hifi.dimensions.scrollbarBackgroundWidth;
    property int connectionsNameCardWidth: connectionsTable.width - locationColumnWidth - actionButtonWidth - 4 - hifi.dimensions.scrollbarBackgroundWidth;
    property var myData: ({profileUrl: "", displayName: "", userName: "", audioLevel: 0.0, avgAudioLevel: 0.0, admin: true, placeName: "", connection: "", isPresent: true}); // valid dummy until set
    property var ignored: ({}); // Keep a local list of ignored avatars & their data. Necessary because HashMap is slow to respond after ignoring.
    property var nearbyUserModelData: []; // This simple list is essentially a mirror of the nearbyUserModel listModel without all the extra complexities.
    property bool iAmAdmin: false;
    property var activeTab: "nearbyTab";
    property bool currentlyEditingDisplayName: false
    property bool punctuationMode: false;
    property double loudSortTime: 0.0;
    readonly property double kLOUD_SORT_PERIOD_MS: 1000.0;

    HifiConstants { id: hifi; }
    RootHttpRequest { id: http; }
    HifiModels.PSFListModel {
        id: connectionsUserModel;
        http: http;
        endpoint: "/api/v1/users/connections";
        property var sortColumn: connectionsTable.getColumn(connectionsTable.sortIndicatorColumn);
        sortProperty: switch (sortColumn && sortColumn.role) {
            case 'placeName':
                'location';
                break;
            case 'connection':
                'is_friend';
                break;
            default:
                'username';
        }
        sortAscending: connectionsTable.sortIndicatorOrder === Qt.AscendingOrder;
        itemsPerPage: 1000;
        listView: connectionsTable;
        processPage: function (data) {
            return data.users.map(function (user) {
                return {
                    userName: user.username,
                    connection: user.connection,
                    profileUrl: user.images.thumbnail,
                    placeName: (user.location.root || user.location.domain || {}).name || ''
                };
            });
        };
    }

    // The letterbox used for popup messages
    LetterboxMessage {
        id: letterboxMessage;
        z: 998; // Force the popup on top of everything else
    }
    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
            myData.userName = Account.username;
            myDataChanged(); // Setting a property within an object isn't enough to update dependencies. This will do it.
        }
    }
    // The ComboDialog used for setting availability
    ComboDialog {
        id: comboDialog;
        z: 998; // Force the ComboDialog on top of everything else
        dialogWidth: parent.width - 50;
        dialogHeight: parent.height - 100;
    }
    function letterbox(headerGlyph, headerText, message) {
        letterboxMessage.headerGlyph = headerGlyph;
        letterboxMessage.headerText = headerText;
        letterboxMessage.text = message;
        letterboxMessage.visible = true;
        letterboxMessage.popupRadius = 0;
    }
    function popupComboDialogCallback(availability) {
        GlobalServices.findableBy = availability;
        UserActivityLogger.palAction("set_availability", availability);
        print('Setting availability:', JSON.stringify(GlobalServices.findableBy));
    }
    function popupComboDialog(dialogTitleText, optionTitleText, optionBodyText, optionValues) {
        comboDialog.callbackFunction = popupComboDialogCallback;
        comboDialog.dialogTitleText = dialogTitleText;
        comboDialog.optionTitleText = optionTitleText;
        comboDialog.optionBodyText = optionBodyText;
        comboDialog.optionValues = optionValues;
        comboDialog.selectedOptionIndex = ['all', 'connections', 'friends', 'none'].indexOf(GlobalServices.findableBy);
        comboDialog.populateComboListViewModel();
        comboDialog.visible = true;
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
    function refreshNearbyWithFilter() {
        // We should just be able to set settings.filtered to inViewCheckbox.checked, but see #3249, so send to .js for saving.
        var userIds = getSelectedNearbySessionIDs();
        var params = {filter: inViewCheckbox.checked && {distance: settings.nearDistance}};
        if (userIds.length > 0) {
            params.selected = [[userIds[0]], true, true];
        }
        pal.sendToScript({method: 'refreshNearby', params: params});
    }
    function refreshConnections() {
        var flickable = connectionsUserModel.flickable;
        connectionsRefreshScrollTimer.oldY = flickable.contentY;
        flickable.contentY = 0;
        connectionsUserModel.getFirstPage('delayRefresh', function () {
            connectionsRefreshScrollTimer.start();
        });
    }
    Timer {
        id: connectionsRefreshScrollTimer;
        interval: 500;
        property real oldY: 0;
        onTriggered: {
            connectionsUserModel.flickable.contentY = oldY;
        }
    }

    Rectangle {
        id: palTabContainer;
        // Anchors
        anchors {
            top: myInfo.bottom;
            bottom: parent.bottom;
            left: parent.left;
            right: parent.right;
        }
        color: "white";
        Rectangle {
            id: tabSelectorContainer;
            // Anchors
            anchors {
                top: parent.top;
                horizontalCenter: parent.horizontalCenter;
            }
            width: parent.width;
            height: 50;
            Rectangle {
                id: nearbyTabSelector;
                // Anchors
                anchors {
                    top: parent.top;
                    left: parent.left;
                }
                width: parent.width/2;
                height: parent.height;
                color: activeTab == "nearbyTab" ? "white" : "#CCCCCC";
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        if (activeTab != "nearbyTab") {
                            refreshNearbyWithFilter();
                        }
                        activeTab = "nearbyTab";
                        connectionsHelpText.color = hifi.colors.baseGray;
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
                        color: activeTab === "nearbyTab" ? hifi.colors.blueAccent : hifi.colors.baseGray;
                        // Alignment
                        horizontalAlignment: Text.AlignHLeft;
                        verticalAlignment: Text.AlignVCenter;
                    }
                    // "In View" Checkbox
                    HifiControlsUit.CheckBox {
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
                        HifiControlsUit.GlyphButton {
                            id: reloadNearby;
                            width: reloadNearby.height;
                            glyph: hifi.glyphs.reload;
                            onClicked: {
                                refreshNearbyWithFilter();
                            }
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
                color: activeTab == "connectionsTab" ? "white" : "#CCCCCC";
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        if (activeTab != "connectionsTab") {
                            connectionsUserModel.getFirstPage();
                        }
                        activeTab = "connectionsTab";
                        connectionsOnlineDot.visible = false;
                        pal.sendToScript({method: 'hideNotificationDot'});
                        connectionsHelpText.color = hifi.colors.blueAccent;
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
                        HifiControlsUit.GlyphButton {
                            id: reloadConnections;
                            width: reloadConnections.height;
                            glyph: hifi.glyphs.reload;
                            onClicked: {
                                pal.sendToScript({method: 'refreshConnections'});
                                refreshConnections();
                            }
                        }
                    }
                    Rectangle {
                        id: connectionsOnlineDot;
                        visible: false;
                        width: 10;
                        height: width;
                        radius: width;
                        color: "#EF3B4E"
                        anchors.left: parent.left;
                        anchors.verticalCenter: parent.verticalCenter;
                    }
                    // "CONNECTIONS" text
                    RalewaySemiBold {
                        id: connectionsTabSelectorText;
                        text: "CONNECTIONS";
                        // Text size
                        size: hifi.fontSizes.tabularData;
                        // Anchors
                        anchors.left: connectionsOnlineDot.visible ? connectionsOnlineDot.right : parent.left;
                        anchors.leftMargin: connectionsOnlineDot.visible ? 4 : 0;
                        anchors.top: parent.top;
                        anchors.bottom: parent.bottom;
                        anchors.right: parent.right;
                        // Style
                        font.capitalization: Font.AllUppercase;
                        color: activeTab === "connectionsTab" ? hifi.colors.blueAccent : hifi.colors.baseGray;
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
                        anchors.leftMargin: connectionsTabSelectorTextMetrics.width + 42 + connectionsOnlineDot.width + connectionsTabSelectorText.anchors.leftMargin;
                        RalewayRegular {
                            id: connectionsHelpText;
                            text: "[?]";
                            size: connectionsTabSelectorText.size + 6;
                            font.capitalization: Font.AllUppercase;
                            color: connectionsTabSelectorText.color;
                            horizontalAlignment: Text.AlignHCenter;
                            verticalAlignment: Text.AlignVCenter;
                            anchors.fill: parent;
                        }
                        MouseArea {
                            anchors.fill: parent;
                            hoverEnabled: true;
                            onClicked: letterbox(hifi.glyphs.question,
                                                 "Connections and Friends",
                                                 "<font color='purple'>Purple borders around profile pictures represent <b>Connections</b>.</font><br>" +
                                                 "When your availability is set to Everyone, Connections can see your username and location.<br><br>" +
                                                 "<font color='green'>Green borders around profile pictures represent <b>Friends</b>.</font><br>" +
                                                 "When your availability is set to Friends, only Friends can see your username and location.");
                            onEntered: connectionsHelpText.color = hifi.colors.blueHighlight;
                            onExited: connectionsHelpText.color = hifi.colors.blueAccent;
                        }
                    }
                }
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
        HifiControlsUit.Table {
            id: nearbyTable;
            flickableItem.interactive: true;
            // Anchors
            anchors.fill: parent;
            // Properties
            centerHeaderText: true;
            sortIndicatorVisible: true;
            headerVisible: true;
            sortIndicatorColumn: settings.nearbySortIndicatorColumn;
            sortIndicatorOrder: settings.nearbySortIndicatorOrder;
            onSortIndicatorColumnChanged: {
                if (sortIndicatorColumn > 2) {
                    // these are not sortable, switch back to last column
                    sortIndicatorColumn = settings.nearbySortIndicatorColumn;
                } else {
                    settings.nearbySortIndicatorColumn = sortIndicatorColumn;
                    sortModel();
                }
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
                width: actionButtonWidth - 8;
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
                height: rowHeight + (styleData.selected ? 15 : 0);
                color: nearbyRowColor(styleData.selected, styleData.alternate);
            }

            // This Item refers to the contents of each Cell
            itemDelegate: Item {
                id: itemCell;
                property bool isCheckBox: styleData.role === "personalMute" || styleData.role === "ignore";
                property bool isButton: styleData.role === "mute" || styleData.role === "kick";
                property bool isBan: styleData.role === "kick";
                property bool isAvgAudio: styleData.role === "avgAudioLevel";
                opacity: !isButton ? (model && model.isPresent ? 1.0 : 0.4) : 1.0; // Admin actions shouldn't turn gray

                // This NameCard refers to the cell that contains an avatar's
                // DisplayName and UserName
                NameCard {
                    id: nameCard;
                    // Properties
                    profileUrl: (model && model.profileUrl) || "";
                    displayName: styleData.value;
                    userName: model ? model.userName : "";
                    connectionStatus: model ? model.connection : "";
                    audioLevel: model ? model.audioLevel : 0.0;
                    avgAudioLevel: model ? model.avgAudioLevel : 0.0;
                    visible: !isCheckBox && !isButton && !isAvgAudio;
                    uuid: model ? model.sessionId : "";
                    selected: styleData.selected;
                    isReplicated: model && model.isReplicated;
                    isAdmin: model && model.admin;
                    isPresent: model && model.isPresent;
                    // Size
                    width: nearbyNameCardWidth;
                    height: parent.height;
                    // Anchors
                    anchors.left: parent.left;
                }
                HifiControlsUit.GlyphButton {
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
                    enabled: (model ? !model["ignore"] && model["isPresent"] : true);
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

                // This CheckBox belongs in the columns that contain the stateful action buttons ("Ignore" for now)
                // KNOWN BUG with the Checkboxes: When clicking in the center of the sorting header, the checkbox
                // will appear in the "hovered" state. Hovering over the checkbox will fix it.
                // Clicking on the sides of the sorting header doesn't cause this problem.
                // I'm guessing this is a QT bug and not anything I can fix. I spent too long trying to work around it...
                // I'm just going to leave the minor visual bug in.
                HifiControlsUit.CheckBox {
                    id: actionCheckBox;
                    visible: isCheckBox;
                    anchors.centerIn: parent;
                    checked: model ? model[styleData.role] : false;
                    // If this is an "Ignore" checkbox, disable the checkbox if user isn't present.
                    enabled: styleData.role === "ignore" ? (model ? model["isPresent"] : true) : true;
                    boxSize: 24;
                    isRedCheck: true
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
                HifiControlsUit.Button {
                    id: actionButton;
                    color: 2; // Red
                    visible: isButton;
                    enabled: !nameCard.isReplicated;
                    anchors.verticalCenter: itemCell.verticalCenter;
                    anchors.left: parent.left;
                    anchors.leftMargin: styleData.role === "kick" ? 1 : 14;
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
                        size: parent.height * 1.3;
                        // Anchors
                        anchors.fill: parent;
                        // Style
                        horizontalAlignment: Text.AlignHCenter;
                        color: enabled ? hifi.buttons.textColor[actionButton.color]
                            : hifi.buttons.disabledTextColor[actionButton.colorScheme];
                    }
                }
                
                HifiControlsUit.Button {
                    id: hardBanButton;
                    color: 2; // Red
                    visible: isBan;
                    enabled: !nameCard.isReplicated;
                    anchors.verticalCenter: itemCell.verticalCenter;
                    anchors.left: parent.left;
                    anchors.leftMargin: actionButton.width + 3;
                    width: 32;
                    height: 32;
                    onClicked: {
                        Users[styleData.role](model.sessionId, Users.BAN_BY_USERNAME | Users.BAN_BY_FINGERPRINT | Users.BAN_BY_IP);
                        UserActivityLogger["palAction"](styleData.role, model.sessionId);
                        if (styleData.role === "kick") {
                            nearbyUserModelData.splice(model.userIndex, 1);
                            nearbyUserModel.remove(model.userIndex); // after changing nearbyUserModelData, b/c ListModel can frob the data
                        }
                    }
                    // muted/error glyphs
                    HiFiGlyphs {
                        text: hifi.glyphs.alert;
                        // Size
                        size: parent.height * 1.3;
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
            id: questionRect
            color: hifi.colors.tableBackgroundLight;
            width: 20;
            height: hifi.dimensions.tableHeaderHeight - 2;
            anchors.left: nearbyTable.left;
            anchors.top: nearbyTable.top;
            anchors.topMargin: 1;

            Connections {
                target: nearbyTable
                onTitlePaintedPosSignal: {
                    if (column === 1) { // name column
                        questionRect.anchors.leftMargin = actionButtonWidth + nearbyTable.titlePaintedPos[column]
                    }
                }
            }

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
                                     "Others can find you and see your username according to your <b>availability</b> settings.<br>" +
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
                                     "<b>Ban (left)</b> identifies a user by username (if applicable) and machine fingerprint, then removes them from this domain and prevents them from returning. Admins can un-ban users from the Server Domain Settings page.<br><br>" +
                                     "<b>Hard Ban (right)</b> identifies a user by username (if applicable), machine fingerprint, and IP address, then removes them from this domain and prevents them from returning. Admins can un-ban users from the Server Domain Settings page.");
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
        color: "white";
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
            anchors.top: parent.top;
            anchors.topMargin: 185;
            anchors.horizontalCenter: parent.horizontalCenter;
            visible: !connectionsUserModel.retrievedAtLeastOnePage;
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
        HifiControlsUit.Table {
            id: connectionsTable;
            flickableItem.interactive: true;
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
            }
            onSortIndicatorOrderChanged: {
                settings.connectionsSortIndicatorOrder = sortIndicatorOrder;
            }

            TableViewColumn {
                id: connectionsUserNameHeader;
                role: "userName";
                title: connectionsUserModel.totalEntries + (connectionsUserModel.totalEntries === 1 ? " NAME" : " NAMES");
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
                role: "connection";
                title: "FRIEND";
                width: actionButtonWidth;
                movable: false;
                resizable: false;
            }

            model: connectionsUserModel;

            // This Rectangle refers to each Row in the connectionsTable.
            rowDelegate: Rectangle {
                // Size
                height: rowHeight + (styleData.selected ? 15 : 0);
                color: connectionsRowColor(styleData.selected, styleData.alternate);
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
                    displayName: "";
                    userName: model ? model.userName : "";
                    placeName: model ? model.placeName : ""
                    connectionStatus : model ? model.connection : "";
                    selected: styleData.selected;
                    // Size
                    width: connectionsNameCardWidth;
                    height: parent.height;
                    // Anchors
                    anchors.left: parent.left;
                }

                // LOCATION data
                FiraSansRegular {
                    id: connectionsLocationData
                    // Properties
                    visible: styleData.role === "placeName";
                    text: (model && model.placeName) || "";
                    elide: Text.ElideRight;
                    // Size
                    width: parent.width;
                    // you would think that this would work:
                    // anchors.verticalCenter: connectionsNameCard.avImage.verticalCenter
                    // but no!  you cannot anchor to a non-sibling or parent.  So I will
                    // align with the friends checkbox, where I did the manual alignment
                    anchors.verticalCenter: friendsCheckBox.verticalCenter
                    // Text Size
                    size: 16;
                    // Text Positioning
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    // Style
                    color: hifi.colors.blueAccent;
                    font.underline: true;
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: enabled
                        enabled: connectionsNameCard.selected && pal.activeTab == "connectionsTab"
                        onClicked: {
                            AddressManager.goToUser(model.userName, false);
                            UserActivityLogger.palAction("go_to_user", model.userName);
                        }
                        onEntered: connectionsLocationData.color = hifi.colors.blueHighlight;
                        onExited: connectionsLocationData.color = hifi.colors.blueAccent;
                    }
                }

                // "Friends" checkbox
                HifiControlsUit.CheckBox {
                    id: friendsCheckBox;
                    visible: styleData.role === "connection" && model && model.userName !== myData.userName;
                    // you would think that this would work:
                    // anchors.verticalCenter: connectionsNameCard.avImage.verticalCenter
                    // but no!  you cannot anchor to a non-sibling or parent.  So:
                    x: parent.width/2 - boxSize/2;
                    y: connectionsNameCard.avImage.y + connectionsNameCard.avImage.height/2 - boxSize/2;
                    checked: model && (model.connection === "friend");
                    boxSize: 24;
                    onClicked: {
                        pal.sendToScript({method: checked ? 'addFriend' : 'removeFriend', params: model.userName});

                        UserActivityLogger["palAction"](checked ? styleData.role : "un-" + styleData.role, model.sessionId);
                    }
                }
            }
        }

        // "Make a Connection" instructions
        Rectangle {
            id: connectionInstructions;
            visible: connectionsTable.rowCount === 0 && !connectionsLoading.visible;
            anchors.fill: connectionsTable;
            anchors.topMargin: hifi.dimensions.tableHeaderHeight;
            color: "white";

            RalewayRegular {
                id: makeAConnectionText;
                // Properties
                text: "Make a Connection";
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 60;
                anchors.left: parent.left;
                anchors.right: parent.right;
                // Text Size
                size: 24;
                // Text Positioning
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter;
                // Style
                color: hifi.colors.darkGray;
            }

            Image {
                id: connectionImage;
                source: "../../icons/connection.svg";
                width: 150;
                height: 150;
                mipmap: true;
                // Anchors
                anchors.top: makeAConnectionText.bottom;
                anchors.topMargin: 15;
                anchors.horizontalCenter: parent.horizontalCenter;
            }

            Text {
                id: connectionHelpText;
                // Anchors
                anchors.top: connectionImage.bottom;
                anchors.topMargin: 15;
                anchors.left: parent.left
                anchors.leftMargin: 40;
                anchors.right: parent.right
                anchors.rightMargin: 10;
                // Text alignment
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHLeft
                // Style
                font.pixelSize: 18;
                font.family: "Raleway"
                color: hifi.colors.darkGray
                wrapMode: Text.WordWrap
                textFormat: Text.StyledText;
                property string hmdMountedInstructions:
                    "1. Put your hand out onto their hand and squeeze your controller's grip button on its side.<br>" +
                    "2. Once the other person puts their hand onto yours, you'll see your connection form.<br>" +
                    "3. After about 3 seconds, you're connected!"
                property string hmdNotMountedInstructions:
                    "1. Press and hold the 'x' key to extend your arm.<br>" +
                    "2. Once the other person puts their hand onto yours, you'll see your connection form.<br>" +
                    "3. After about 3 seconds, you're connected!";
                property string notLoggedInInstructions: "<b><font color='red'>You must be logged into your High Fidelity account to make connections.</b></font><br>"
                property string instructions:
                    "<b>When you meet someone you want to remember later, you can <font color='purple'>connect</font> with a handshake:</b><br><br>"
                // Text
                text:
                    Account.isLoggedIn() ? ( HMD.mounted ? instructions + hmdMountedInstructions : instructions + hmdNotMountedInstructions)
                    : ( HMD.mounted ? notLoggedInInstructions + instructions + hmdMountedInstructions : notLoggedInInstructions + instructions + hmdNotMountedInstructions)
            }

        }
    } // "Connections" Tab
    } // palTabContainer

    // This contains the current user's NameCard and will contain other information in the future
    Rectangle {
        id: myInfo;
        // Size
        width: pal.width;
        height: myCardHeight;
        // Style
        color: pal.color;
        // Anchors
        anchors.top: pal.top;
        anchors.topMargin: 10;
        anchors.left: pal.left;
        // This NameCard refers to the current user's NameCard (the one above the nearbyTable)
        NameCard {
            id: myCard;
            // Properties
            profileUrl: myData.profileUrl;
            displayName: myData.displayName;
            userName: myData.userName;
            audioLevel: myData.audioLevel;
            avgAudioLevel: myData.avgAudioLevel;
            isMyCard: true;
            isPresent: true;
            // Size
            width: myCardWidth;
            height: parent.height;
            // Anchors
            anchors.top: parent.top
            anchors.left: parent.left;
        }
        Item {
            id: upperRightInfoContainer;
            width: 200;
            height: parent.height;
            anchors.top: parent.top;
            anchors.right: parent.right;

            RalewayRegular {
                id: availabilityText;
                text: "set availability";
                // Text size
                size: hifi.fontSizes.tabularData;
                // Anchors
                anchors.left: parent.left;
                // Style
                color: hifi.colors.baseGrayHighlight;
                // Alignment
                horizontalAlignment: Text.AlignLeft;
                verticalAlignment: Text.AlignTop;
            }
            Rectangle {
                property var availabilityStrings: ["Everyone", "Friends and Connections", "Friends Only", "Appear Offline"];
                id: availabilityComboBox;
                color: hifi.colors.textFieldLightBackground
                // Anchors
                anchors.top: availabilityText.bottom;
                anchors.horizontalCenter: parent.horizontalCenter;
                // Size
                width: parent.width;
                height: 40;
                function determineAvailabilityIndex() {
                    return ['all', 'connections', 'friends', 'none'].indexOf(GlobalServices.findableBy);
                }

                function determineAvailabilityString() {
                    return availabilityStrings[determineAvailabilityIndex()];
                }
                RalewayRegular {
                    text: myData.userName === "Unknown user" ? "Login to Set" : availabilityComboBox.determineAvailabilityString();
                    anchors.fill: parent;
                    anchors.leftMargin: 10;
                    horizontalAlignment: Text.AlignLeft;
                    size: 16;
                }
                MouseArea {
                    anchors.fill: parent;
                    enabled: myData.userName !== "Unknown user" && !userInfoViewer.visible;
                    hoverEnabled: true;
                    onClicked: {
                        // TODO: Change language from "Happening Now" to something else (or remove entirely)
                        popupComboDialog("Set your availability:",
                        availabilityComboBox.availabilityStrings,
                        ["Your username will be visible in everyone's 'Nearby' list. Anyone will be able to jump to your location from within the 'Nearby' list.",
                        "Your location will be visible in the 'Connections' list only for those with whom you are connected or friends. They'll be able to jump to your location if the domain allows, and you will see 'Snaps' Blasts from them in 'Go To'.",
                        "Your location will be visible in the 'Connections' list only for those with whom you are friends. They'll be able to jump to your location if the domain allows, and you will see 'Snaps' Blasts from them in 'Go To'",
                        "You will appear offline in the 'Connections' list, and you will not receive Snaps Blasts from connections or friends in 'Go To'."],
                        ["all", "connections", "friends", "none"]);
                    }
                    onEntered: availabilityComboBox.color = hifi.colors.lightGrayText;
                    onExited: availabilityComboBox.color = hifi.colors.textFieldLightBackground;
                }
            }
        }
    }

        HifiControlsUit.Keyboard {
            id: keyboard;
            raised: currentlyEditingDisplayName && HMD.mounted;
            numeric: parent.punctuationMode;
            anchors {
                bottom: parent.bottom;
                left: parent.left;
                right: parent.right;
            }
        } // Keyboard

        HifiControls.TabletWebView {
            id: userInfoViewer;
            z: 999;
            anchors {
                top: parent.top;
                bottom: parent.bottom;
                left: parent.left;
                right: parent.right;
            }
            visible: false;
        }

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

    function nearbyRowColor(selected, alternate) {
        return selected ? hifi.colors.orangeHighlight : alternate ? hifi.colors.tableRowLightEven : hifi.colors.tableRowLightOdd;
    }
    function connectionsRowColor(selected, alternate) {
        return selected ? hifi.colors.lightBlueHighlight : alternate ? hifi.colors.tableRowLightEven : hifi.colors.tableRowLightOdd;
    }
    function findNearbySessionIndex(sessionId, optionalData) { // no findIndex in .qml
        var data = optionalData || nearbyUserModelData, length = data.length;
        for (var i = 0; i < length; i++) {
            if (data[i].sessionId === sessionId.toString()) {
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
            iAmAdmin = Users.canKick;
            index = findNearbySessionIndex("", data);
            if (index !== -1) {
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
            reloadNearby.color = 0;
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
                    letterbox('', '', 'The user you attempted to select is no longer available.');
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
            // in any case make sure we are in the nearby tab
            activeTab="nearbyTab";
            break;
        // Received an "updateUsername()" request from the JS
        case 'updateUsername':
            // Get the connection status
            var connectionStatus = message.params.connection;
            // If the connection status isn't "self"...
            if (connectionStatus !== "self") {
                // Get the index in nearbyUserModel and nearbyUserModelData associated with the passed UUID
                var userIndex = findNearbySessionIndex(message.params.sessionId);
                if (userIndex !== -1) {
                    ['userName', 'admin', 'connection', 'profileUrl', 'placeName'].forEach(function (name) {
                        var value = message.params[name];
                        if (value === undefined || value == "") {
                            return;
                        }
                        nearbyUserModel.setProperty(userIndex, name, value);
                        nearbyUserModelData[userIndex][name] = value; // for refill after sort
                    });
                }
            } else if (message.params.profileUrl) {
                myData.profileUrl = message.params.profileUrl;
                myCard.profileUrl = message.params.profileUrl;
            }
            break;
        case 'updateAudioLevel':
            for (var userId in message.params) {
                var audioLevel = message.params[userId][0];
                var avgAudioLevel = message.params[userId][1];
                // If the userId is "", we're updating "myData".
                if (userId === "") {
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
            if (nearbyTable.sortIndicatorColumn == 0 && Date.now() - pal.loudSortTime >= pal.kLOUD_SORT_PERIOD_MS) {
                // Current sort by loudness so re-sort.
                sortModel();
                pal.loudSortTime = Date.now();
            }
            break;
        case 'clearLocalQMLData':
            ignored = {};
            break;
        case 'refreshConnections':
            refreshConnections();
            break;
        case 'connectionRemoved':
            for (var i=0; i<connectionsUserModel.count; ++i) {
                if (connectionsUserModel.get(i).userName === message.params) {
                    connectionsUserModel.remove(i);
                    break;
                }
            }
            break;
        case 'avatarDisconnected':
            var sessionID = message.params[0];
            delete ignored[sessionID];
            break;
        case 'palIsStale':
            var sessionID = message.params[0];
            var reason = message.params[1];
            var userIndex = findNearbySessionIndex(sessionID);
            if (userIndex != -1) {
                if (!nearbyUserModelData[userIndex].ignore) {
                    if (reason !== 'avatarAdded') {
                        nearbyUserModel.setProperty(userIndex, "isPresent", false);
                        nearbyUserModelData[userIndex].isPresent = false;
                        nearbyTable.selection.deselect(userIndex);
                    }
                    reloadNearby.color = 2;
                }
            } else {
                reloadNearby.color = 2;
            }
            break;
        case 'inspectionCertificate_resetCert':
            // marketplaces.js sends out a signal to QML with that method when the tablet screen changes and it's not changed to a commerce-related screen.
            // We want it to only be handled by the InspectionCertificate.qml, but there's not an easy way of doing that.
            // As a part of a "cleanup inspectionCertificate_resetCert" ticket, we'll have to figure out less logspammy way of doing what has to be done.
            break;
        case 'http.response':
            http.handleHttpResponse(message);
            break;
        case 'changeConnectionsDotStatus':
            connectionsOnlineDot.visible = message.shouldShowDot;
            break;
        default:
            console.log('Pal.qml: Unrecognized message');
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
