import Hifi 1.0 as Hifi
import QtQuick 2.3
import QtQuick.Controls 1.2

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
        readonly property int fontSize: 12
        readonly property string fontColor: "white"
        readonly property string bgColor: "#99333333"

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
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Servers: " + root.serverCount
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Avatars: " + root.avatarCount
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Frame Rate: " + root.framerate.toFixed(2);
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Render Rate: " + root.renderrate.toFixed(2);
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Present Rate: " + root.presentrate.toFixed(2);
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Present New Rate: " + root.presentnewrate.toFixed(2);
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Present Drop Rate: " + root.presentdroprate.toFixed(2);
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Simrate: " + root.simrate
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Avatar Simrate: " + root.avatarSimrate
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Packets In/Out: " + root.packetInCount + "/" + root.packetOutCount
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Mbps In/Out: " + root.mbpsIn.toFixed(2) + "/" + root.mbpsOut.toFixed(2)
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
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
                    Text {
                        color: root.fontColor
                        font.pixelSize: root.fontSize
                        text: "Audio ping: " + root.audioPing
                    }
                    Text {
                        color: root.fontColor
                        font.pixelSize: root.fontSize
                        text: "Avatar ping: " + root.avatarPing
                    }
                    Text {
                        color: root.fontColor
                        font.pixelSize: root.fontSize
                        text: "Entities avg ping: " + root.entitiesPing
                    }
                    Text {
                        color: root.fontColor
                        font.pixelSize: root.fontSize
                        text: "Asset ping: " + root.assetPing
                    }
                    Text {
                        color: root.fontColor
                        font.pixelSize: root.fontSize
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
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Position: " + root.position.x.toFixed(1) + ", " +
                            root.position.y.toFixed(1) + ", " + root.position.z.toFixed(1)
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Speed: " + root.speed.toFixed(1)
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Yaw: " + root.yaw.toFixed(1)
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: "Avatar Mixer In: " + root.avatarMixerInKbps + " kbps, " +
                            root.avatarMixerInPps + "pps";
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: "Avatar Mixer Out: " + root.avatarMixerOutKbps + " kbps, " +
                            root.avatarMixerOutPps + "pps";
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: "Downloads: " + root.downloads + "/" + root.downloadLimit +
                              ", Pending: " + root.downloadsPending;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded && root.downloadUrls.length > 0;
                        text: "Download URLs:"
                    }
                    ListView {
                        width: geoCol.width
                        height: root.downloadUrls.length * 15

                        visible: root.expanded && root.downloadUrls.length > 0;

                        model: root.downloadUrls
                        delegate: Text {
                            color: root.fontColor;
                            font.pixelSize: root.fontSize
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
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Triangles: " + root.triangles +
                            " / Material Switches: " + root.materialSwitches
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "GPU Textures: " + root.gpuTextures;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "GPU Buffers: " + root.gpuBuffers;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: "Items rendered / considered: " +
                            root.itemRendered + " / " + root.itemConsidered;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: " out of view: " + root.itemOutOfView +
                            " too small: " + root.itemTooSmall;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: "Shadows rendered / considered: " +
                            root.shadowRendered + " / " + root.shadowConsidered;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: " out of view: " + root.shadowOutOfView +
                            " too small: " + root.shadowTooSmall;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: !root.expanded
                        text: "Octree Elements Server: " + root.serverElements +
                            " Local: " + root.localElements;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded
                        text: "Octree Sending Mode: " + root.sendingMode;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded
                        text: "Octree Packets to Process: " + root.packetStats;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded
                        text: "Octree Elements - ";
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded
                        text: "\tServer: " + root.serverElements +
                            " Internal: " + root.serverInternal +
                            " Leaves: " + root.serverLeaves;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded
                        text: "\tLocal: " + root.localElements +
                            " Internal: " + root.localInternal +
                            " Leaves: " + root.localLeaves;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
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
            Text {
                x: 4; y: 4
                id: perfText
                color: root.fontColor
                font.family: root.monospaceFont
                font.pixelSize: 12
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
