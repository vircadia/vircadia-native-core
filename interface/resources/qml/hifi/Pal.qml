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
import "../styles-uit" // fixme should end up removeable

Rectangle {
    id: pal;
    property int keepFromHorizontalScroll: 1;
    width: parent.width - keepFromHorizontalScroll;
    height: parent.height;

    property int nameWidth: width/2;
    property int actionWidth: nameWidth / (table.columnCount - 1);
    property int rowHeight: 50;
    property var userData: [];
    property var myData: ({displayName: "", userName: ""}); // valid dummy until set
    function fromScript(data) {
        var myIndex = 0;
        while ((myIndex < data.length) && data[myIndex].sessionId) myIndex++; // no findIndex in .qml
        myData = data[myIndex];
        data.splice(myIndex, 1);
        userData = data;
        console.log('FIXME', JSON.stringify(myData), myIndex, JSON.stringify(userData));
        sortModel();
    }
    ListModel {
        id: userModel
    }
    function sortModel() {
        var sortProperty = table.getColumn(table.sortIndicatorColumn).role;
        var before = (table.sortIndicatorOrder === Qt.AscendingOrder) ? -1 : 1;
        var after = -1 * before;
        console.log('fixme sort', table.sortIndicatorColumn, sortProperty, before, after);
        userData.sort(function (a, b) {
            var aValue = a[sortProperty].toString().toLowerCase(), bValue = b[sortProperty].toString().toLowerCase();
            switch (true) {
            case (aValue < bValue): console.log('fixme', aValue, bValue, before); return before;
            case (aValue > bValue): console.log('fixme', aValue, bValue, after); return after;
            default: return 0;
            }
        });
        userModel.clear();
        var userIndex = 0;
        userData.forEach(function (datum) {
            function init(property) {
                if (datum[property] === undefined) {
                    datum[property] = false;
                }
            }
            ['mute', 'spacer', 'ignore', 'ban'].forEach(init);
            datum.userIndex = userIndex++;
            userModel.append(datum);
        });
    }
    signal sendToScript(var message);
    function noticeSelection() {
        console.log('selection changed');
        var userIds = [];
        table.selection.forEach(function (userIndex) {
            userIds.push(userData[userIndex].sessionId);
        });
        console.log('fixme selected ' + JSON.stringify(userIds));
        pal.sendToScript(userIds);
        //pal.parent.sendToScript(userIds);
    }
    Connections {
        target: table.selection
        onSelectionChanged: pal.noticeSelection()
    }

    Column {
        NameCard {
            id: myCard;
            width: nameWidth;
            displayName: myData.displayName;
            userName: myData.userName;
        }
        TableView {
            id: table;
            TableViewColumn {
                role: "displayName";
                title: "Name";
                width: nameWidth
            }
            TableViewColumn {
                role: "ignore";
                title: "Ignore"
                width: actionWidth
            }
            TableViewColumn {
                title: "";
                width: actionWidth
            }
            TableViewColumn {
                role: "mute";
                title: "Mute";
                width: actionWidth
            }
            TableViewColumn {
                role: "kick";
                title: "Ban"
                width: actionWidth
            }
            model: userModel;
            rowDelegate: Rectangle { // The only way I know to specify a row height.
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
            itemDelegate: Item {
                id: itemCell;
                property bool isCheckBox: typeof(styleData.value) === 'boolean';
                NameCard {
                    id: nameCard;
                    visible: !isCheckBox;
                    width: nameWidth;
                    displayName: styleData.value;
                    userName: model.userName;
                }
                Rectangle {
                    radius: itemCell.height / 4;
                    visible: isCheckBox;
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
            height: pal.height - myCard.height;
            width: pal.width;
            sortIndicatorVisible: true;
            onSortIndicatorColumnChanged: sortModel();
            onSortIndicatorOrderChanged: sortModel();
        }
    }
}
