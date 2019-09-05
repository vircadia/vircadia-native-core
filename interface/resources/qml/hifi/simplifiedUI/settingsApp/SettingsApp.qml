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
import QtQuick.Controls 2.3
import "../simplifiedConstants" as SimplifiedConstants
import "../simplifiedControls" as SimplifiedControls
import stylesUit 1.0 as HifiStylesUit
import "./audio" as AudioSettings
import "./general" as GeneralSettings
import "./vr" as VrSettings
import "./dev" as DevSettings
Rectangle {
    property string activeTabView: "generalTabView"
    id: root
    color: simplifiedUI.colors.darkBackground
    anchors.fill: parent
    property bool developerModeEnabled: Settings.getValue("simplifiedUI/developerModeEnabled", false)

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }
            
    focus: true        
    Keys.onPressed: {
        if ((event.key == Qt.Key_D) && (event.modifiers & Qt.ControlModifier && event.modifiers & Qt.AltModifier && event.modifiers & Qt.ShiftModifier)) {
            var currentSetting = Settings.getValue("simplifiedUI/developerModeEnabled", false);
            var newSetting = !currentSetting;
            Settings.setValue("simplifiedUI/developerModeEnabled", newSetting);
            root.developerModeEnabled = newSetting;
            if (newSetting) {
                console.log("Developer mode ON. You are now a developer!");
            } else {
                console.log("Developer mode OFF.");
                if (root.activeTabView === "devTabView") {
                    tabListView.currentIndex = 2;
                    root.activeTabView = "vrTabView";
                }
            }
        }
    }

    onActiveTabViewChanged: {
        for (var i = 0; i < tabListModel.count; i++) {
            if (tabListModel.get(i).tabViewName === activeTabView) {
                tabListView.currentIndex = i;
                return;
            }
        }
    }

    Component.onCompleted: {
        root.forceActiveFocus();
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
            ListElement {
                tabTitle: "Dev"
                tabViewName: "devTabView"
            }
        }


        Component {
            id: highlightBar
            Rectangle {
                width: tabListView.currentItem.width
                height: tabListView.currentItem.height
                color: simplifiedUI.colors.darkBackground
                x: tabListView.currentItem.x
                Behavior on x {
                    SmoothedAnimation {
                        duration: 250
                    }
                }
                Behavior on width {
                    SmoothedAnimation {
                        duration: 250
                    }
                }
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
            highlightFollowsCurrentItem: false
            interactive: contentItem.width > width
            delegate: Item {
                visible: model.tabTitle !== "Dev" || (model.tabTitle === "Dev" && root.developerModeEnabled)

                width: tabTitleText.paintedWidth + 32
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
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        GeneralSettings.General {
            id: generalTabViewContainer
            visible: activeTabView === "generalTabView"
            anchors.fill: parent
            onSendNameTagInfo: {
                sendToScript(message);
            }
            onSendEmoteVisible: {
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

        DevSettings.Dev {
            id: devTabViewContainer
            visible: activeTabView === "devTabView"
            anchors.fill: parent
        }

        SimplifiedControls.VerticalScrollBar {
            parent: {
                if (activeTabView === "generalTabView") {
                    generalTabViewContainer
                } else if (activeTabView === "audioTabView") {
                    audioTabViewContainer
                } else if (activeTabView === "vrTabView") {
                    vrTabViewContainer
                } else if (activeTabView === "devTabView") {
                    devTabViewContainer
                }
            }
        }
    }


    function fromScript(message) {
        if (message.source !== "simplifiedUI.js") {
            return;
        }

        switch (message.method) {
            case "goToSettingsTab":
                var tabToGoTo = message.data.settingsTab;
                switch (tabToGoTo) {
                    case "audio":
                        activeTabView = "audioTabView";
                        break;

                    default:
                        console.log("A message was sent to the Settings window to change tabs, but an invalid tab was specified.");
                        break;
                }
                break;

            default:
                console.log('SettingsApp.qml: Unrecognized message from JS');
                break;
        }
    }
    signal sendToScript(var message);
}
