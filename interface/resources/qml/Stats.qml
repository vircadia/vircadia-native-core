import Hifi 1.0 as Hifi
import QtQuick 2.3
import QtQuick.Controls 1.2
import '.'

Item {
    id: stats

    anchors.leftMargin: 300
    objectName: "StatsItem"

    Component.onCompleted: {
        stats.parentChanged.connect(fill);
        fill();
    }
    Component.onDestruction: {
        stats.parentChanged.disconnect(fill);
    }

    function fill() {
        // Explicitly fill in order to avoid warnings at shutdown
        anchors.fill = parent;
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
                        text: "Frame Rate: " + root.framerate.toFixed(2);
                    }
                    StatText {
                        text: "Render Rate: " + root.renderrate.toFixed(2);
                    }
                    StatText {
                        text: "Present Rate: " + root.presentrate.toFixed(2);
                    }
                    StatText {
                        text: "Present New Rate: " + root.presentnewrate.toFixed(2);
                    }
                    StatText {
                        text: "Present Drop Rate: " + root.presentdroprate.toFixed(2);
                    }
                    StatText {
                        text: "Stutter Rate: " + root.stutterrate.toFixed(3);
                        visible: root.stutterrate != -1;
                    }
                    StatText {
                        text: "Simrate: " + root.simrate
                    }
                    StatText {
                        text: "Avatar Simrate: " + root.avatarSimrate
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
                        text: "Audio ping: " + root.audioPing
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
                            root.avatarMixerOutPps + "pps";
                    }
                    StatText {
                        visible: root.expanded;
                        text: "Downloads: " + root.downloads + "/" + root.downloadLimit +
                              ", Pending: " + root.downloadsPending;
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
                        text: "  Frame timing:"
                    }
                    StatText {
                        text: "      Batch: " + root.batchFrameTime.toFixed(1) + " ms"
                    }
                    StatText {
                        text: "      GPU: " + root.gpuFrameTime.toFixed(1) + " ms"
                    }
                    StatText {
                        text: "Triangles: " + root.triangles +
                            " / Material Switches: " + root.materialSwitches
                    }
                    StatText {
                        text: "GPU Free Memory: " + root.gpuFreeMemory + " MB";
                    }
                    StatText {
                        text: "GPU Textures: ";
                    }
                    StatText {
                        text: "  Sparse Enabled: " + (0 == root.gpuSparseTextureEnabled ? "false" : "true");
                    }
                    StatText {
                        text: "  Count: " + root.gpuTextures;
                    }
                    StatText {
                        text: "  Rectified: " + root.rectifiedTextureCount;
                    }
                    StatText {
                        text: "  Decimated: " + root.decimatedTextureCount;
                    }
                    StatText {
                        text: "  Sparse Count: " + root.gpuTexturesSparse;
                        visible: 0 != root.gpuSparseTextureEnabled;
                    }
                    StatText {
                        text: "  Virtual Memory: " + root.gpuTextureVirtualMemory + " MB";
                    }
                    StatText {
                        text: "  Commited Memory: " + root.gpuTextureMemory + " MB";
                    }
                    StatText {
                        text: "  Framebuffer Memory: " + root.gpuTextureFramebufferMemory + " MB";
                    }
                    StatText {
                        text: "  Sparse Memory: " + root.gpuTextureSparseMemory + " MB";
                        visible: 0 != root.gpuSparseTextureEnabled;
                    }
                    StatText {
                        text: "GPU Buffers: "
                    }
                    StatText {
                        text: "  Count: " + root.gpuTextures;
                    }
                    StatText {
                        text: "  Memory: " + root.gpuBufferMemory;
                    }
                    StatText {
                        text: "GL Swapchain Memory: " + root.glContextSwapchainMemory + " MB";
                    }
                    StatText {
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
