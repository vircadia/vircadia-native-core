//
//  ComboDialog.qml
//  qml/hifi
//
//  Created by Zach Fox on 3/31/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import "../styles-uit"

Item {
    property var dialogTitleText;
    property var optionTitleText;
    property var optionBodyText;
    property var optionValues;
    property var selectedOptionIndex;
    property int dialogWidth;
    property int dialogHeight;
    property int comboOptionTextSize: 18;
    FontLoader { id: ralewayRegular; source: "../../fonts/Raleway-Regular.ttf"; }
    FontLoader { id: ralewaySemiBold; source: "../../fonts/Raleway-SemiBold.ttf"; }
    visible: false;
    id: combo;
    anchors.fill: parent;
    onVisibleChanged: {
        populateComboListViewModel();
    }

    Rectangle {
        id: dialogBackground;
        anchors.fill: parent;
        color: "black";
        opacity: 0.5;
    }

    Rectangle {
        id: dialogContainer;
        color: "white";
        anchors.centerIn: dialogBackground;
        width: dialogWidth;
        height: dialogHeight;

        RalewayRegular {
            id: dialogTitle;
            text: dialogTitleText;
            anchors.top: parent.top;
            anchors.topMargin: 20;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            size: 24;
            color: 'black';
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignTop;
        }

        ListModel {
            id: comboListViewModel;
        }

        ListView {
            id: comboListView;
            anchors.top: dialogTitle.bottom;
            anchors.topMargin: 20;
            anchors.bottom: parent.bottom;
            anchors.left: parent.left;
            anchors.right: parent.right;
            model: comboListViewModel;
            delegate: comboListViewDelegate;

            Component {
                id: comboListViewDelegate;
                Rectangle {
                    id: comboListViewItemContainer;
                    // Size
                    height: childrenRect.height + 10;
                    width: dialogContainer.width;
                    color: selectedOptionIndex === index ? '#cee6ff' : 'white';
                    Rectangle {
                        id: comboOptionSelected;
                        visible: selectedOptionIndex === index ? true : false;
                        color: hifi.colors.blueAccent;
                        anchors.left: parent.left;
                        anchors.leftMargin: 20;
                        anchors.top: parent.top;
                        anchors.topMargin: 20;
                        width: 25;
                        height: width;
                        radius: width;
                        border.width: 3;
                        border.color: hifi.colors.blueHighlight;
                    }


                    RalewaySemiBold {
                        id: optionTitle;
                        text: titleText;
                        anchors.top: parent.top;
                        anchors.left: comboOptionSelected.right;
                        anchors.leftMargin: 20;
                        anchors.right: parent.right;
                        height: 30;
                        size: comboOptionTextSize;
                        wrapMode: Text.WordWrap;
                    }

                    RalewayRegular {
                        id: optionBody;
                        text: bodyText;
                        anchors.top: optionTitle.bottom;
                        anchors.bottom: parent.bottom;
                        anchors.left: comboOptionSelected.right;
                        anchors.leftMargin: 25;
                        anchors.right: parent.right;
                        size: comboOptionTextSize;
                        wrapMode: Text.WordWrap;
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true;
                        onEntered: comboListViewItemContainer.color = hifi.colors.blueHighlight
                        onExited: comboListViewItemContainer.color = selectedOptionIndex === index ? '#cee6ff' : 'white';
                        onClicked: {
                            GlobalServices.findableBy = optionValue;
                            UserActivityLogger.palAction("set_availability", optionValue);
                            print('Setting availability:', optionValue);
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onClicked: {
            combo.visible = false;
        }
    }

    function populateComboListViewModel() {
        comboListViewModel.clear();
        optionTitleText.forEach(function(titleText, index) {
            comboListViewModel.insert(index, {"titleText": titleText, "bodyText": optionBodyText[index], "optionValue": optionValues[index]});
        });
    }
}