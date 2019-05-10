//
//  SettingsApp.qml
//
//  Created by Zach Fox on 2019-05-02
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import "../simplifiedConstants" as SimplifiedConstants
import stylesUit 1.0 as HifiStylesUit
import "./audio" as AudioSettings
import "./general" as GeneralSettings
import "./vr" as VrSettings

Rectangle {
    property string activeTabView: "generalTabView"
    id: root
    color: simplifiedUI.colors.darkBackground
    anchors.fill: parent

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }


    Rectangle {
        id: tabContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        color: simplifiedUI.colors.highlightOnDark

        ListModel {
            id: tabListModel

            ListElement {
                tabTitle: "General"
                tabViewName: "generalTabView"
            }
            ListElement {
                tabTitle: "Audio"
                tabViewName: "audioTabView"
            }
            ListElement {
                tabTitle: "VR"
                tabViewName: "vrTabView"
            }
        }


        Component {
            id: highlightBar
            Rectangle {
                color: simplifiedUI.colors.darkBackground
            }
        }


        ListView {
            id: tabListView
            anchors.fill: parent
            contentHeight: parent.height
            contentWidth: childrenRect.width
            orientation: ListView.Horizontal
            model: tabListModel
            highlight: highlightBar
            interactive: contentItem.width > width
            delegate: Item {
                width: tabTitleText.paintedWidth + 64
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    id: tabTitleText
                    color: simplifiedUI.colors.text.white
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: model.tabTitle
                    size: 24
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        tabListView.currentIndex = index;
                        root.activeTabView = model.tabViewName;
                    }
                }
            }
        }
    }

    Item {
        id: tabViewContainers
        anchors.top: tabContainer.bottom
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.bottom: parent.bottom


        GeneralSettings.General {
            id: generalTabViewContainer
            visible: activeTabView === "generalTabView"
            anchors.fill: parent
            onSendNameTagInfo: {
                sendToScript(message);
            }
        }
        

        AudioSettings.Audio {
            id: audioTabViewContainer
            visible: activeTabView === "audioTabView"
            anchors.fill: parent
        }

        VrSettings.VR {
            id: vrTabViewContainer
            visible: activeTabView === "vrTabView"
            anchors.fill: parent
        }
    }

    Image {
        source: "../images/accent.svg"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 94
        height: 175
    }


    function fromScript(message) {
        switch (message.method) {
            default:
                console.log('SettingsApp.qml: Unrecognized message from JS');
                break;
        }
    }
    signal sendToScript(var message);
}
