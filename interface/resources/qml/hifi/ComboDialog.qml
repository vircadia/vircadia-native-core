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
import "../controls-uit"

Item {
    property var dialogTitleText : "";
    property var optionTitleText: "";
    property var optionBodyText: "";
    property var optionValues: [];
    property var selectedOptionIndex: 0;
    property var callbackFunction;
    property int dialogWidth;
    property int dialogHeight;
    property int comboOptionTextSize: 16;
    property int comboBodyTextSize: 16;
    FontLoader { id: ralewayRegular; source: "../../fonts/Raleway-Regular.ttf"; }
    FontLoader { id: ralewaySemiBold; source: "../../fonts/Raleway-SemiBold.ttf"; }
    visible: false;
    id: combo;
    anchors.fill: parent;
    onVisibleChanged: {
        populateComboListViewModel();
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onClicked: {
            combo.visible = false;
        }
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
        width: combo.dialogWidth;
        height: combo.dialogHeight;

        RalewayRegular {
            id: dialogTitle;
            text: combo.dialogTitleText;
            anchors.top: parent.top;
            anchors.topMargin: 20;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            size: 24;
            color: hifi.colors.darkGray;
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignTop;
        }

        HiFiGlyphs {
            id: closeGlyphButton;
            text: hifi.glyphs.close;
            size: 32;
            anchors.verticalCenter: dialogTitle.verticalCenter;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            MouseArea {
                anchors.fill: closeGlyphButton;
                hoverEnabled: true;
                onEntered: {
                    parent.text = hifi.glyphs.closeInverted;
                }
                onExited: {
                    parent.text = hifi.glyphs.close;
                }
                onClicked: {
                    combo.visible = false;
                }
            }
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
            clip: true;
            model: comboListViewModel;
            delegate: comboListViewDelegate;

            Component {
                id: comboListViewDelegate;
                Rectangle {
                    id: comboListViewItemContainer;
                    // Size
                    height: optionTitle.height + optionBody.height + 20;
                    width: dialogContainer.width;
                    color: selectedOptionIndex === index ? '#cee6ff' : 'white';
                    Rectangle {
                        id: comboOptionSelected;
                        color: selectedOptionIndex == index ? hifi.colors.blueAccent : 'white';
                        anchors.left: parent.left;
                        anchors.leftMargin: 20;
                        anchors.top: parent.top;
                        anchors.topMargin: 20;
                        width: 25;
                        height: width;
                        radius: width;
                        border.width: 3;
                        border.color: selectedOptionIndex === index ? hifi.colors.blueHighlight: hifi.colors.lightGrayText;
                    }


                    RalewaySemiBold {
                        id: optionTitle;
                        text: titleText;
                        anchors.top: parent.top;
                        anchors.topMargin: 7;
                        anchors.left: comboOptionSelected.right;
                        anchors.leftMargin: 10;
                        anchors.right: parent.right;
                        anchors.rightMargin: 10;
                        height: 30;
                        size: comboOptionTextSize;
                        wrapMode: Text.WordWrap;
                        color: hifi.colors.darkGray;
                    }

                    RalewayRegular {
                        id: optionBody;
                        text: bodyText;
                        anchors.top: optionTitle.bottom;
                        anchors.left: comboOptionSelected.right;
                        anchors.leftMargin: 10;
                        anchors.right: parent.right;
                        anchors.rightMargin: 10;
                        size: comboBodyTextSize;
                        wrapMode: Text.WordWrap;
                        color: hifi.colors.darkGray;
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true;
                        onEntered: comboListViewItemContainer.color = hifi.colors.blueHighlight
                        onExited: comboListViewItemContainer.color = selectedOptionIndex === index ? '#cee6ff' : 'white';
                        onClicked: {
                            callbackFunction(optionValue);
                            combo.visible = false;
                        }
                    }
                }
            }
        }
    }

    function populateComboListViewModel() {
        comboListViewModel.clear();
        optionTitleText.forEach(function(titleText, index) {
            comboListViewModel.insert(index, {"titleText": titleText, "bodyText": optionBodyText[index], "optionValue": optionValues[index]});
        });
    }
}
