//
//  About.qml
//
//  Created by Zach Fox on 2019-06-18
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
    contentHeight: aboutColumnLayout.height
    clip: true

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
        source: "../images/accent3.svg"
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
        id: aboutColumnLayout
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        spacing: simplifiedUI.margins.settings.spacingBetweenSettings
        
        ColumnLayout {
            id: platformInfoContainer
            Layout.preferredWidth: parent.width
            Layout.topMargin: 24
            Layout.bottomMargin: 24
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                text: "About Interface"
                Layout.maximumWidth: parent.width
                Layout.bottomMargin: 14
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>Interface Version:</b> " + Window.checkVersion()
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikSemiBold {
                text: "Platform Info"
                Layout.maximumWidth: parent.width
                Layout.topMargin: 24
                Layout.bottomMargin: 14
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>Profiled Platform Tier:</b> " + PlatformInfo.getTierProfiled()
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>OS Type:</b> " + PlatformInfo.getOperatingSystemType()
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>CPU:</b> " + PlatformInfo.getCPUBrand()
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b># CPUs:</b> " + PlatformInfo.getNumCPUs()
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b># CPU Cores:</b> " + PlatformInfo.getNumLogicalCores()
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>RAM:</b> " + PlatformInfo.getTotalSystemMemoryMB() + " MB"
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>GPU:</b> " + PlatformInfo.getGraphicsCardType()
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>VR Hand Controllers:</b> " + (PlatformInfo.hasRiftControllers() ? "Rift" : (PlatformInfo.hasViveControllers() ? "Vive" : "None"))
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            SimplifiedControls.Button {
                Layout.topMargin: 24
                width: 200
                height: 32
                text: "Copy to Clipboard"

                onClicked: {
                    Window.copyToClipboard(root.buildPlatformInfoTextToCopy());
                }
            }
        }
    }

    function buildPlatformInfoTextToCopy() {
        var textToCopy = "About Interface:\n";
        textToCopy += "Interface Version: " + Window.checkVersion() + "\n";
        textToCopy += "\nPlatform Info:\n";
        textToCopy += "Profiled Platform Tier: " + PlatformInfo.getTierProfiled() + "\n";
        textToCopy += "OS Type: " + PlatformInfo.getOperatingSystemType() + "\n";
        textToCopy += "CPU: " + PlatformInfo.getCPUBrand() + "\n";
        textToCopy += "# CPUs: " + PlatformInfo.getNumCPUs() + "\n";
        textToCopy += "# CPU Cores: " + PlatformInfo.getNumLogicalCores() + "\n";
        textToCopy += "RAM: " + PlatformInfo.getTotalSystemMemoryMB() + " MB\n";
        textToCopy += "GPU: " + PlatformInfo.getGraphicsCardType() + "\n";
        textToCopy += "VR Hand Controllers: " + (PlatformInfo.hasRiftControllers() ? "Rift" : (PlatformInfo.hasViveControllers() ? "Vive" : "None"));

        return textToCopy;
    }
}
