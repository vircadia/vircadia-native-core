import Hifi 1.0 as Hifi
import QtQuick 2.3
import QtQuick.Controls 1.2

Item {
    anchors.fill: parent
    anchors.leftMargin: 300
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
                        text: "Framerate: " + root.framerate
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Simrate: " + root.simrate
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
                        text: "Voxel max ping: " + 0
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
                        text: "Velocity: " + root.velocity.toFixed(1)
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
                        text: "Avatar Mixer: " + root.avatarMixerKbps + " kbps, " +
                            root.avatarMixerPps + "pps";
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: "Downloads: ";
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
                }
                Column {
                    id: octreeCol
                    spacing: 4; x: 4; y: 4;
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        text: "Triangles: " + root.triangles +
                            " / Quads: " + root.quads + " / Material Switches: " + root.materialSwitches
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: "\tMesh Parts Rendered Opaque: " + root.meshOpaque +
                            " / Translucent: " + root.meshTranslucent;
                    }
                    Text {
                        color: root.fontColor;
                        font.pixelSize: root.fontSize
                        visible: root.expanded;
                        text: "\tOpaque considered: " + root.opaqueConsidered +
                            " / Out of view: " + root.opaqueOutOfView + " / Too small: " + root.opaqueTooSmall;
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
