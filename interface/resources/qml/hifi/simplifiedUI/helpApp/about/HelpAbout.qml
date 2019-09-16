//
//  HelpAbout.qml
//
//  Created by Zach Fox on 2019-08-07
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

            // When the user clicks the About tab, refresh the audio I/O model so that
            // the delegate Component.onCompleted handlers fire, which will update
            // the text that appears in the About screen.
            audioOutputDevices.model = undefined;
            audioOutputDevices.model = AudioScriptingInterface.devices.output;
            audioInputDevices.model = undefined;
            audioInputDevices.model = AudioScriptingInterface.devices.input;
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
        spacing: 0
        
        ColumnLayout {
            id: platformInfoContainer
            Layout.preferredWidth: parent.width
            Layout.bottomMargin: 24
            spacing: 0

            HifiStylesUit.GraphikSemiBold {
                text: "About Your Configuration"
                Layout.preferredWidth: parent.width
                Layout.topMargin: 16
                Layout.bottomMargin: 8
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "Use the button below to get a copy to share with us."
                Layout.preferredWidth: parent.width
                Layout.bottomMargin: 8
                height: paintedHeight
                size: 18
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "Version " + Window.checkVersion()
                Layout.preferredWidth: parent.width
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikSemiBold {
                text: "Platform Info"
                Layout.preferredWidth: parent.width
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                height: paintedHeight
                size: 22
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>Computer Vendor/Model:</b>"
                Layout.preferredWidth: parent.width
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
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>OS Type:</b> " + PlatformInfo.getOperatingSystemType()
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>CPU:</b>"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
                
                Component.onCompleted: {
                    var cpu = JSON.parse(PlatformInfo.getCPU(0));
                    var cpuModel = cpu.model;
                    if (!cpuModel || cpuModel.length === 0) {
                        cpuModel = "Unknown";
                    }

                    text = "<b>CPU:</b> " + cpuModel;
                }
            }

            HifiStylesUit.GraphikRegular {
                text: "<b># CPUs:</b> " + PlatformInfo.getNumCPUs()
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b># CPU Cores:</b> " + PlatformInfo.getNumLogicalCores()
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>RAM:</b> " + PlatformInfo.getTotalSystemMemoryMB() + " MB"
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>GPU:</b> "
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
                
                Component.onCompleted: {
                    var gpu = JSON.parse(PlatformInfo.getGPU(PlatformInfo.getMasterGPU()));
                    var gpuModel = gpu.model;
                    if (!gpuModel || gpuModel.length === 0) {
                        gpuModel = "Unknown";
                    }

                    text = "<b>GPU:</b> " + gpuModel;
                }
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>VR Hand Controllers:</b> " + (PlatformInfo.hasRiftControllers() ? "Rift" : (PlatformInfo.hasViveControllers() ? "Vive" : "None"))
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }

            // This is a bit of a hack to get the name of the currently-selected audio input device
            // in the current mode (Desktop or VR). The reason this is necessary is because it seems to me like
            // the only way one can get a human-readable list of the audio I/O devices is by using a ListView
            // and grabbing the names from the AudioScriptingInterface; you can't do it using a ListModel.
            // See `AudioDevices.h`, specifically the comment above the declaration of `QVariant data()`.
            ListView {
                id: audioInputDevices
                visible: false
                property string selectedInputDeviceName
                Layout.preferredWidth: parent.width
                Layout.preferredHeight: contentItem.height
                interactive: false
                delegate: Item {
                    Component.onCompleted: {
                        if ((HMD.active && selectedHMD) || (!HMD.active && selectedDesktop)) {
                            audioInputDevices.selectedInputDeviceName = model.devicename
                        }
                    }
                }
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>Audio Input:</b> " + audioInputDevices.selectedInputDeviceName
                Layout.preferredWidth: parent.width
                height: paintedHeight
                size: 16
                color: simplifiedUI.colors.text.white
                wrapMode: Text.Wrap
            }


            // This is a bit of a hack to get the name of the currently-selected audio output device
            // in the current mode (Desktop or VR). The reason this is necessary is because it seems to me like
            // the only way one can get a human-readable list of the audio I/O devices is by using a ListView
            // and grabbing the names from the AudioScriptingInterface; you can't do it using a ListModel.
            // See `AudioDevices.h`, specifically the comment above the declaration of `QVariant data()`.
            ListView {
                id: audioOutputDevices
                visible: false
                property string selectedOutputDeviceName
                Layout.preferredWidth: parent.width
                Layout.preferredHeight: contentItem.height
                interactive: false
                delegate: Item {
                    Component.onCompleted: {
                        if ((HMD.active && selectedHMD) || (!HMD.active && selectedDesktop)) {
                            audioOutputDevices.selectedOutputDeviceName = model.devicename
                        }
                    }
                }
            }

            HifiStylesUit.GraphikRegular {
                text: "<b>Audio Output:</b> " + audioOutputDevices.selectedOutputDeviceName
                Layout.preferredWidth: parent.width
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
        if (!cpuModel || cpuModel.length === 0) {
            cpuModel = "Unknown";
        }

        textToCopy += "CPU: " + cpuModel + "\n";
        textToCopy += "# CPUs: " + PlatformInfo.getNumCPUs() + "\n";
        textToCopy += "# CPU Cores: " + PlatformInfo.getNumLogicalCores() + "\n";
        textToCopy += "RAM: " + PlatformInfo.getTotalSystemMemoryMB() + " MB\n";
        
        var gpu = JSON.parse(PlatformInfo.getGPU(PlatformInfo.getMasterGPU()));
        var gpuModel = gpu.model;
        if (!gpuModel || gpuModel.length === 0) {
            gpuModel = "Unknown";
        }

        textToCopy += "GPU: " + gpuModel + "\n";
        textToCopy += "VR Hand Controllers: " + (PlatformInfo.hasRiftControllers() ? "Rift" : (PlatformInfo.hasViveControllers() ? "Vive" : "None")) + "\n";
        textToCopy += "Audio Input: " + audioInputDevices.selectedInputDeviceName + "\n";
        textToCopy += "Audio Output: " + audioOutputDevices.selectedOutputDeviceName + "\n";
        
        textToCopy += "\n**All Platform Info**\n";
        textToCopy += JSON.stringify(JSON.parse(PlatformInfo.getPlatform()), null, 4);

        return textToCopy;
    }
}
