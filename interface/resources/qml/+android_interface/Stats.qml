import Hifi 1.0 as Hifi
import QtQuick 2.3
import '.'

Item {
    id: stats

    anchors.leftMargin: 300
    objectName: "StatsItem"
    property int modality: Qt.NonModal
    implicitHeight: row.height
    implicitWidth: row.width
    visible: false

    Component.onCompleted: {
        stats.parentChanged.connect(fill);
        fill();
    }
    Component.onDestruction: {
        stats.parentChanged.disconnect(fill);
    }

    function fill() {
        // This will cause a  warning at shutdown, need to find another way to remove
        // the warning other than filling the anchors to the parent
        anchors.horizontalCenter = parent.horizontalCenter
    }

    Hifi.Stats {
        id: root
        objectName: "Stats"
        implicitHeight: row.height
        implicitWidth: row.width

        anchors.horizontalCenter: parent.horizontalCenter
        readonly property string bgColor: "#AA111111"

        Row {
            id: row
            spacing: 8
            Rectangle {
                width: generalCol.width + 8;
                height: generalCol.height + 8;
                color: root.bgColor;

                MouseArea {
                    anchors.fill: parent
                    onClicked: { root.expanded = !root.expanded; }
                    hoverEnabled: true
                }

                Column {
                    id: generalCol
                    spacing: 4; x: 4; y: 4;
                    StatText {
                        text: "Servers: " + root.serverCount
                    }
                    StatText {
                        text: "Avatars: " + root.avatarCount
                    }
                    StatText {
                        text: "Game Rate: " + root.gameLoopRate
                    }
                    StatText {
                        visible: root.expanded
                        text: root.gameUpdateStats
                    }
                    StatText {
                        text: "Render Rate: " + root.renderrate.toFixed(2);
                    }
                    StatText {
                        text: "Present Rate: " + root.presentrate.toFixed(2);
                    }
                    StatText {
                        visible: root.expanded
                        text: "    Present New Rate: " + root.presentnewrate.toFixed(2);
                    }
                    StatText {
                        visible: root.expanded
                        text: "    Present Drop Rate: " + root.presentdroprate.toFixed(2);
                    }
                    StatText {
                        text: "Stutter Rate: " + root.stutterrate.toFixed(3);
                        visible: root.stutterrate != -1;
                    }
                    StatText {
                        text: "Missed Frame Count: " + root.appdropped;
                        visible: root.appdropped > 0;
                    }
                    StatText {
                        text: "Long Render Count: " + root.longrenders;
                        visible: root.longrenders > 0;
                    }
                    StatText {
                        text: "Long Submit Count: " + root.longsubmits;
                        visible: root.longsubmits > 0;
                    }
                    StatText {
                        text: "Long Frame Count: " + root.longframes;
                        visible: root.longframes > 0;
                    }
                    StatText {
                        text: "Packets In/Out: " + root.packetInCount + "/" + root.packetOutCount
                    }
                    StatText {
                        text: "Mbps In/Out: " + root.mbpsIn.toFixed(2) + "/" + root.mbpsOut.toFixed(2)
                    }
                    StatText {
                        visible: root.expanded
                        text: "Asset Mbps In/Out: " + root.assetMbpsIn.toFixed(2) + "/" + root.assetMbpsOut.toFixed(2)
                    }
                    StatText {
                        visible: root.expanded
                        text: "Avatars Updated: " + root.updatedAvatarCount
                    }
                    StatText {
                        visible: root.expanded
                        text: "Heroes Count/Updated: " + root.heroAvatarCount + "/" + root.updatedHeroAvatarCount
                    }
                    StatText {
                        visible: root.expanded
                        text: "Avatars NOT Updated: " + root.notUpdatedAvatarCount
                    }
                }
            }

            Rectangle {
                width: pingCol.width + 8
                height: pingCol.height + 8
                color: root.bgColor;
                visible: root.audioPing != -2
                MouseArea {
                    anchors.fill: parent
                    onClicked: { root.expanded = !root.expanded; }
                    hoverEnabled: true
                }
                Column {
                    id: pingCol
                    spacing: 4; x: 4; y: 4;
                    StatText {
                        text: "Audio ping/loss: " + root.audioPing + "/" + root.audioPacketLoss + "%"
                    }
                    StatText {
                        text: "Avatar ping: " + root.avatarPing
                    }
                    StatText {
                        text: "Entities avg ping: " + root.entitiesPing
                    }
                    StatText {
                        text: "Asset ping: " + root.assetPing
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Messages max ping: " + root.messagePing
                    }
                }
            }

            Rectangle {
                width: geoCol.width + 8
                height: geoCol.height + 8
                color: root.bgColor;
                MouseArea {
                    anchors.fill: parent
                    onClicked: { root.expanded = !root.expanded; }
                    hoverEnabled: true
                }
                Column {
                    id: geoCol
                    spacing: 4; x: 4; y: 4;
                    StatText {
                        text: "Position: " + root.position.x.toFixed(1) + ", " +
                            root.position.y.toFixed(1) + ", " + root.position.z.toFixed(1)
                    }
                    StatText {
                        text: "Speed: " + root.speed.toFixed(1)
                    }
                    StatText {
                        text: "Yaw: " + root.yaw.toFixed(1)
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Avatar Mixer In: " + root.avatarMixerInKbps + " kbps, " +
                            root.avatarMixerInPps + "pps";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Avatar Mixer Out: " + root.avatarMixerOutKbps + " kbps, " +
                            root.avatarMixerOutPps + "pps, " +
                            root.myAvatarSendRate.toFixed(2) + "hz";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Audio Mixer In: " + root.audioMixerInKbps + " kbps, " +
                            root.audioMixerInPps + "pps";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Audio Mixer Out: " + root.audioMixerOutKbps + " kbps, " +
                        root.audioMixerOutPps + "pps";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Audio In Audio: " + root.audioInboundPPS + " pps, " +
                            "Silent: " + root.audioSilentInboundPPS + " pps";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Audio Out Mic: " + root.audioOutboundPPS + " pps, " +
                            "Silent: " + root.audioSilentOutboundPPS + " pps";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Audio Codec: " + root.audioCodec + " Noise Gate: " +
                            root.audioNoiseGate;
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Entity Servers In: " + root.entityPacketsInKbps + " kbps";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Downloads: " + root.downloads + "/" + root.downloadLimit +
                              ", Pending: " + root.downloadsPending;
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Processing: " + root.processing +
                              ", Pending: " + root.processingPending;
                    }
                    StatText {
                        visible: root.expanded && root.downloadUrls.length > 0;
                        text: "Download URLs:"
                    }
                    ListView {
                        width: geoCol.width
                        height: root.downloadUrls.length * 15

                        visible: root.expanded && root.downloadUrls.length > 0;

                        model: root.downloadUrls
                        delegate: StatText {
                            visible: root.expanded;
                            text: modelData.length > 30
                                ?  modelData.substring(0, 5) + "..." + modelData.substring(modelData.length - 22)
                                : modelData
                        }
                    }
                }
            }
            Rectangle {
                width: octreeCol.width + 8
                height: octreeCol.height + 8
                color: root.bgColor;
                MouseArea {
                    anchors.fill: parent
                    onClicked: { root.expanded = !root.expanded; }
                    hoverEnabled: true
                }
                Column {
                    id: octreeCol
                    spacing: 4; x: 4; y: 4;
                    StatText {
                        text: "Render Engine: " + root.engineFrameTime.toFixed(1) + " ms"
                    }
                    StatText {
                        text: "Batch: " + root.batchFrameTime.toFixed(1) + " ms"
                    }
                    StatText {
                        text: "GPU: " + root.gpuFrameTime.toFixed(1) + " ms"
                    }
                    StatText {
                        text: "LOD Target: " + root.lodTargetFramerate + " Hz Angle: " + root.lodAngle + " deg"
                    }
                    StatText {
                        text: "Drawcalls: " + root.drawcalls
                    }
                    StatText {
                        text: "Triangles: " + root.triangles +
                            " / Material Switches: " + root.materialSwitches
                    }
                    StatText {
                        visible: root.expanded;
                        text: "GPU Free Memory: " + root.gpuFreeMemory + " MB";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "GPU Textures: ";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "  Count: " + root.gpuTextures;
                    }
                    StatText {
                        visible: root.expanded;
                        text: "  Pressure State: " + root.gpuTextureMemoryPressureState;
                    }
                    StatText {
                        text: "  Resource Allocated / Populated / Pending: ";
                    }
                    StatText {
                        text: "       " + root.gpuTextureResourceMemory + " / " + root.gpuTextureResourcePopulatedMemory + " / " + root.texturePendingTransfers + " MB";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "  Resident Memory: " + root.gpuTextureResidentMemory + " MB";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "  Framebuffer Memory: " + root.gpuTextureFramebufferMemory + " MB";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "  External Memory: " + root.gpuTextureExternalMemory + " MB";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "GPU Buffers: "
                    }
                    StatText {
                        visible: root.expanded;
                        text: "  Count: " + root.gpuBuffers;
                    }
                    StatText {
                        visible: root.expanded;
                        text: "  Memory: " + root.gpuBufferMemory + " MB";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "GL Swapchain Memory: " + root.glContextSwapchainMemory + " MB";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "QML Texture Memory: " + root.qmlTextureMemory + " MB";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Items rendered / considered: " +
                            root.itemRendered + " / " + root.itemConsidered;
                    }
                    StatText {
                        visible: root.expanded;
                        text: " out of view: " + root.itemOutOfView +
                            " too small: " + root.itemTooSmall;
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Shadows rendered / considered: " +
                            root.shadowRendered + " / " + root.shadowConsidered;
                    }
                    StatText {
                        visible: root.expanded;
                        text: " out of view: " + root.shadowOutOfView +
                            " too small: " + root.shadowTooSmall;
                    }
                    StatText {
                        visible: !root.expanded
                        text: "Octree Elements Server: " + root.serverElements +
                            " Local: " + root.localElements;
                    }
                    StatText {
                        visible: root.expanded
                        text: "Octree Sending Mode: " + root.sendingMode;
                    }
                    StatText {
                        visible: root.expanded
                        text: "Octree Packets to Process: " + root.packetStats;
                    }
                    StatText {
                        visible: root.expanded
                        text: "Octree Elements - ";
                    }
                    StatText {
                        visible: root.expanded
                        text: "\tServer: " + root.serverElements +
                            " Internal: " + root.serverInternal +
                            " Leaves: " + root.serverLeaves;
                    }
                    StatText {
                        visible: root.expanded
                        text: "\tLocal: " + root.localElements +
                            " Internal: " + root.localInternal +
                            " Leaves: " + root.localLeaves;
                    }
                    StatText {
                        visible: root.expanded
                        text: "LOD: " + root.lodStatus;
                    }
                    StatText {
                        visible: root.expanded
                        text: "Entity Updates: " + root.numEntityUpdates + " / " + root.numNeededEntityUpdates;
                    }
                }
            }
        }

        Rectangle {
            y: 250
            visible: root.timingExpanded
            width: perfText.width + 8
            height: perfText.height + 8
            color: root.bgColor;
            StatText {
                x: 4; y: 4
                id: perfText
                font.family: root.monospaceFont
                text: "------------------------------------------ Function " +
                    "--------------------------------------- --msecs- -calls--\n" +
                    root.timingStats;
            }
        }

        Connections {
            target: root.parent
            onWidthChanged: {
                root.x = root.parent.width - root.width;
            }
        }
    }

}
