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


    ColumnLayout {
        id: aboutColumnLayout
        anchors.left: parent.left
        anchors.leftMargin: 26
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.top: parent.top
        spacing: 0

        Image {
            source: "images/logo.png"
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 16
            Layout.preferredWidth: 200
            Layout.preferredHeight: 150
            fillMode: Image.PreserveAspectFit
            mipmap: true
        }
        
        ColumnLayout {
            id: platformInfoContainer
            Layout.preferredWidth: parent.width
            Layout.bottomMargin: 24
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                text: "Version " + Window.checkVersion()
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikSemiBold {
                text: "Platform Info"
                Layout.maximumWidth: parent.width
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>Computer Vendor/Model:</b>"
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap

                Component.onCompleted: {
                    var computer = JSON.parse(PlatformInfo.getComputer());
                    var computerVendor = computer.vendor;
                    if (computerVendor.length === 0) {
                        computerVendor = "Unknown";
                    }
                    var computerModel = computer.model;
                    if (computerModel.length === 0) {
                        computerModel = "Unknown";
                    }

                    text = "<b>Computer Vendor/Model:</b> " + computerVendor + "/" + computerModel;
                }
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
                text: "<b>CPU:</b>"
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
                
                Component.onCompleted: {
                    var cpu = JSON.parse(PlatformInfo.getCPU(0));
                    var cpuModel = cpu.model;
                    if (cpuModel.length === 0) {
                        cpuModel = "Unknown";
                    }

                    text = "<b>CPU:</b> " + cpuModel;
                }
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
                text: "<b>GPU:</b> "
                Layout.maximumWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
                
                Component.onCompleted: {
                    var gpu = JSON.parse(PlatformInfo.getGPU(0));
                    var gpuModel = gpu.model;
                    if (gpuModel.length === 0) {
                        gpuModel = "Unknown";
                    }

                    text = "<b>GPU:</b> " + gpuModel;
                }
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
                Layout.topMargin: 8
                width: 200
                height: 32
                text: "Copy to Clipboard"
                temporaryText: "Copied!"

                onClicked: {
                    Window.copyToClipboard(root.buildPlatformInfoTextToCopy());
                    showTemporaryText();
                }
            }
        }
    }

    function buildPlatformInfoTextToCopy() {
        var textToCopy = "**About Interface**\n";
        textToCopy += "Interface Version: " + Window.checkVersion() + "\n";
        textToCopy += "\n**Platform Info**\n";

        var computer = JSON.parse(PlatformInfo.getComputer());
        var computerVendor = computer.vendor;
        if (computerVendor.length === 0) {
            computerVendor = "Unknown";
        }
        var computerModel = computer.model;
        if (computerModel.length === 0) {
            computerModel = "Unknown";
        }

        textToCopy += "Computer Vendor/Model: " + computerVendor + "/" + computerModel + "\n";
        textToCopy += "Profiled Platform Tier: " + PlatformInfo.getTierProfiled() + "\n";
        textToCopy += "OS Type: " + PlatformInfo.getOperatingSystemType() + "\n";

        var cpu = JSON.parse(PlatformInfo.getCPU(0));
        var cpuModel = cpu.model;
        if (cpuModel.length === 0) {
            cpuModel = "Unknown";
        }

        textToCopy += "CPU: " + cpuModel + "\n";
        textToCopy += "# CPUs: " + PlatformInfo.getNumCPUs() + "\n";
        textToCopy += "# CPU Cores: " + PlatformInfo.getNumLogicalCores() + "\n";
        textToCopy += "RAM: " + PlatformInfo.getTotalSystemMemoryMB() + " MB\n";
        
        var gpu = JSON.parse(PlatformInfo.getGPU(0));
        var gpuModel = gpu.model;
        if (gpuModel.length === 0) {
            gpuModel = "Unknown";
        }

        textToCopy += "GPU: " + gpuModel + "\n";
        textToCopy += "VR Hand Controllers: " + (PlatformInfo.hasRiftControllers() ? "Rift" : (PlatformInfo.hasViveControllers() ? "Vive" : "None")) + "\n";
        
        textToCopy += "\n**All Platform Info**\n";
        textToCopy += JSON.stringify(JSON.parse(PlatformInfo.getPlatform()), null, 4);

        return textToCopy;
    }
}
