//
//  General.qml
//
//  Created by Zach Fox on 2019-05-06
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import "../../simplifiedConstants" as SimplifiedConstants
import "../../simplifiedControls" as SimplifiedControls
import stylesUit 1.0 as HifiStylesUit
import QtQuick.Layouts 1.3
import PerformanceEnums 1.0

Flickable {
    property string avatarNametagMode: Settings.getValue("simplifiedNametag/avatarNametagMode", "on")
    id: root
    contentWidth: parent.width
    contentHeight: generalColumnLayout.height
    clip: true

    onAvatarNametagModeChanged: {
        sendNameTagInfo({method: 'handleAvatarNametagMode', avatarNametagMode: root.avatarNametagMode, source: "SettingsApp.qml"});
    }

    onVisibleChanged: {
        if (visible) {
            root.contentX = 0;
            root.contentY = 0;
        }
    }

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }


    Image {
        id: accent
        source: "../images/accent1.svg"
        anchors.left: parent.left
        anchors.top: parent.top
        width: 83
        height: 156
        transform: Scale {
            xScale: -1
            origin.x: accent.width / 2
            origin.y: accent.height / 2
        }
    }


    ColumnLayout {
        id: generalColumnLayout
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        spacing: simplifiedUI.margins.settings.spacingBetweenSettings

        ColumnLayout {
            id: avatarNameTagsContainer
            Layout.preferredWidth: parent.width
            Layout.topMargin: 24
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                id: avatarNameTagsTitle
                text: "Avatar Name Tags"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            ColumnLayout {
                id: avatarNameTagsRadioButtonGroup
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                spacing: simplifiedUI.margins.settings.spacingBetweenRadiobuttons

                SimplifiedControls.RadioButton {
                    id: avatarNameTagsOff
                    text: "Off"
                    checked: root.avatarNametagMode === "off"
                    onClicked: {
                        root.avatarNametagMode = "off"
                    }
                }

                SimplifiedControls.RadioButton {
                    id: avatarNameTagsAlwaysOn
                    text: "Always On"
                    checked: root.avatarNametagMode === "alwaysOn"
                    onClicked: {
                        root.avatarNametagMode = "alwaysOn"
                    }
                }

                SimplifiedControls.RadioButton {
                    id: avatarNameTagsClickToView
                    text: "Click to View"
                    checked: root.avatarNametagMode === "on"
                    onClicked: {
                        root.avatarNametagMode = "on"
                    }
                }
            }
        }

        ColumnLayout {
            id: emoteContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                    id: emoteTitle
                    text: "Emote UI"
                    Layout.maximumWidth: parent.width
                    height: paintedHeight
                    size: 22
                    color: simplifiedUI.colors.text.white
                }

            ColumnLayout {
                id: emoteSwitchGroup
                Layout.preferredWidth: parent.width
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin

                SimplifiedControls.Switch {
                    id: emoteSwitch
                    Layout.preferredHeight: 18
                    Layout.preferredWidth: parent.width
                    labelTextOn: "Show Emote UI"
                    checked: Settings.getValue("simplifiedUI/allowEmoteDrawerExpansion", true)
                    onClicked: {
                        var currentSetting = Settings.getValue("simplifiedUI/allowEmoteDrawerExpansion", true);
                        Settings.setValue("simplifiedUI/allowEmoteDrawerExpansion", !currentSetting);
                    }                    

                    Connections {
                        target: Settings

                        onValueChanged: {
                            if (setting === "simplifiedUI/allowEmoteDrawerExpansion") {
                                emoteSwitch.checked = value;
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            id: performanceContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                id: performanceTitle
                text: "Graphics Settings"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            ColumnLayout {
                id: performanceRadioButtonGroup
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                spacing: simplifiedUI.margins.settings.spacingBetweenRadiobuttons

                SimplifiedControls.RadioButton {
                    id: performanceLow
                    text: "Low Power Quality" + (PlatformInfo.getTierProfiled() === PerformanceEnums.LOW_POWER ? " (Recommended)" : "")
                    checked: Performance.getPerformancePreset() === PerformanceEnums.LOW_POWER
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.LOW_POWER);
                    }
                }

                SimplifiedControls.RadioButton {
                    id: performanceLow
                    text: "Low Quality" + (PlatformInfo.getTierProfiled() === PerformanceEnums.LOW ? " (Recommended)" : "")
                    checked: Performance.getPerformancePreset() === PerformanceEnums.LOW
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.LOW);
                    }
                }

                SimplifiedControls.RadioButton {
                    id: performanceMedium
                    text: "Medium Quality" + (PlatformInfo.getTierProfiled() === PerformanceEnums.MID ? " (Recommended)" : "")
                    checked: Performance.getPerformancePreset() === PerformanceEnums.MID
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.MID);
                    }
                }

                SimplifiedControls.RadioButton {
                    id: performanceHigh
                    text: "High Quality" + (PlatformInfo.getTierProfiled() === PerformanceEnums.HIGH ? " (Recommended)" : "")
                    checked: Performance.getPerformancePreset() === PerformanceEnums.HIGH
                    onClicked: {
                        Performance.setPerformancePreset(PerformanceEnums.HIGH);
                    }
                }
            }
        }

        ColumnLayout {
            id: cameraContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                id: cameraTitle
                text: "Camera View"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            ColumnLayout {
                id: cameraRadioButtonGroup
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin
                spacing: simplifiedUI.margins.settings.spacingBetweenRadiobuttons

                SimplifiedControls.RadioButton {
                    id: firstPerson
                    text: "First Person View"
                    checked: Camera.mode === "first person look at"
                    onClicked: {
                        Camera.mode = "first person look at"
                    }
                }

                SimplifiedControls.RadioButton {
                    id: thirdPerson
                    text: "Third Person View"
                    checked: Camera.mode === "look at"
                    onClicked: {
                        Camera.mode = "look at"
                    }
                }

                SimplifiedControls.RadioButton {
                    id: selfie
                    text: "Selfie"
                    checked: Camera.mode === "selfie"
                    visible: true
                    onClicked: {
                        Camera.mode = "selfie"
                    }
                }
                
                Connections {
                    target: Camera

                    onModeUpdated: {
                        if (Camera.mode === "first person look at") {
                            firstPerson.checked = true
                        } else if (Camera.mode === "look at") {
                            thirdPerson.checked = true
                        } else if (Camera.mode === "selfie" && HMD.active) {
                            selfie.checked = true
                        }
                    }
                }

                Connections {
                    target: HMD

                    onDisplayModeChanged: {
                        selfie.visible = isHMDMode ? false : true
                    }
                }
            }
        }

        ColumnLayout {
            id: logoutContainer
            Layout.preferredWidth: parent.width
            Layout.bottomMargin: 24
            spacing: 0

            HifiStylesUit.GraphikRegular {
                id: logoutText
                text: (AccountServices.username === "Unknown user" ? "Log In" : "Logout " + AccountServices.username)
                wrapMode: Text.Wrap
                width: paintedWidth
                height: paintedHeight
                size: 14
                color: simplifiedUI.colors.text.lightBlue

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        parent.color = simplifiedUI.colors.text.lightBlueHover;
                    }
                    onExited: {
                        parent.color = simplifiedUI.colors.text.lightBlue;
                    }
                    onClicked: {
                        if (Account.loggedIn) {
                            AccountServices.logOut();
                        } else {
                            DialogsManager.showLoginDialog();
                        }
                    }
                }
            }
        }
    }

    signal sendNameTagInfo(var message);
    signal sendEmoteVisible(var message);
}
