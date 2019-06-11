//
//  Dev.qml
//
//  Created by Zach Fox on 2019-06-11
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

Flickable {
    id: root
    contentWidth: parent.width
    contentHeight: devColumnLayout.height
    topMargin: 24
    bottomMargin: 24
    clip: true

    onVisibleChanged: {
        if (visible) {
            root.contentX = 0;
            root.contentY = -root.topMargin;
        }
    }


    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    ColumnLayout {
        id: devColumnLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: simplifiedUI.margins.settings.spacingBetweenSettings
        
        ColumnLayout {
            id: uiControlsContainer
            Layout.preferredWidth: parent.width
            spacing: 0

            HifiStylesUit.GraphikRegular {
                id: uiControlsTitle
                text: "User Interface"
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            HifiStylesUit.GraphikRegular {
                id: uiControlsSubtitle
                text: "You'll have to restart Interface after changing either of these settings. If you don't get any Toolbar apps back after restarting, run defaultScripts.js manually."
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            ColumnLayout {
                id: uiControlsSwitchGroup
                Layout.topMargin: simplifiedUI.margins.settings.settingsGroupTopMargin

                SimplifiedControls.Switch {
                    id: keepMenusSwitch
                    width: parent.width
                    height: 18
                    labelTextOn: "Keep Old Menus (File, Edit, etc)"
                    checked: Settings.getValue("simplifiedUI/keepMenus", false);
                    onClicked: {
                        Settings.setValue("simplifiedUI/keepMenus", !Settings.getValue("simplifiedUI/keepMenus", false));
                    }
                }

                SimplifiedControls.Switch {
                    id: keepOldUIAndScriptsSwitch
                    width: parent.width
                    height: 18
                    labelTextOn: "Keep Old UI and Scripts"
                    checked: Settings.getValue("simplifiedUI/keepExistingUIAndScripts", false);
                    onClicked: {
                        Settings.setValue("simplifiedUI/keepExistingUIAndScripts", !Settings.getValue("simplifiedUI/keepExistingUIAndScripts", false));
                    }
                }
            }
        }
    }
}
