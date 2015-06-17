import Hifi 1.0 as Hifi
import QtQuick 2.3
import QtQuick.Controls 1.2

Hifi.Stats {
    id: root
    objectName: "Stats"
    implicitHeight: row.height
    implicitWidth: row.width
    readonly property int sTATS_GENERAL_MIN_WIDTH: 165
    readonly property int sTATS_PING_MIN_WIDTH: 190
    readonly property int sTATS_GEO_MIN_WIDTH: 240
    readonly property int sTATS_OCTREE_MIN_WIDTH: 410

    onParentChanged: {
        root.x = parent.width - root.width;
    }

    Row {
        z: 100
        id: row
        spacing: 8
        Rectangle {
            width: generalCol.width + 8;
            height: generalCol.height + 8;
            color: "#99333333";

            MouseArea {
                anchors.fill: parent
                onClicked: { root.expanded = !root.expanded; }
            }
    
            Column {
                id: generalCol
                spacing: 4; x: 4; y: 4;
                width: sTATS_GENERAL_MIN_WIDTH
                Text { color: "white"; text: "Servers: " + root.serverCount }
                Text { color: "white"; text: "Avatars: " + root.avatarCount }
                Text { color: "white"; text: "Framerate: " + root.framerate }
                Text { color: "white"; text: "Packets In/Out: " + root.packetInCount + "/" + root.packetOutCount }
                Text { color: "white"; text: "Mbps In/Out: " + root.mbpsIn.toFixed(2) + "/" + root.mbpsOut.toFixed(2) }
            }
        }

        Rectangle {
            width: pingCol.width + 8
            height: pingCol.height + 8
            color: "#99333333"
            visible: root.audioPing != -2
            MouseArea {
                anchors.fill: parent
                onClicked: { root.expanded = !root.expanded; }
            }
            Column {
                id: pingCol
                spacing: 4; x: 4; y: 4;
                width: sTATS_PING_MIN_WIDTH
                Text { color: "white"; text: "Audio ping: " + root.audioPing }
                Text { color: "white"; text: "Avatar ping: " + root.avatarPing }
                Text { color: "white"; text: "Entities avg ping: " + root.entitiesPing }
                Text { color: "white"; text: "Voxel max ping: " + 0; visible: root.expanded; }
            }
        }
        Rectangle {
            width: geoCol.width
            height: geoCol.height
            color: "#99333333"
            MouseArea {
                anchors.fill: parent
                onClicked: { root.expanded = !root.expanded; }
            }
            Column {
                spacing: 4
                id: geoCol
                width: sTATS_GEO_MIN_WIDTH
                Text { 
                    color: "white"; 
                    text: "Position: " + root.position.x.toFixed(1) + ", " + 
                        root.position.y.toFixed(1) + ", " + root.position.z.toFixed(1) 
                }
                Text { 
                    color: "white"; 
                    text: "Velocity: " + root.velocity.toFixed(1) 
                }
                Text { 
                    color: "white"; 
                    text: "Yaw: " + root.yaw.toFixed(1) 
                }
                Text { 
                    color: "white"; 
                    visible: root.expanded; 
                    text: "Avatar Mixer: " + root.avatarMixerKbps + " kbps, " + 
                        root.avatarMixerPps + "pps"; 
                }
                Text { 
                    color: "white"; 
                    visible: root.expanded; 
                    text: "Downloads: "; 
                }
            }
        }
        Rectangle {
            width: octreeCol.width + 8
            height: octreeCol.height + 8
            color: "#99333333"
            MouseArea {
                anchors.fill: parent
                onClicked: { root.expanded = !root.expanded; }
            }
            Column {
                id: octreeCol
                spacing: 4; x: 4; y: 4;
                width: sTATS_OCTREE_MIN_WIDTH
                Text { 
                    color: "white"; 
                    text: "Triangles: " + root.triangles + 
                        " / Quads: " + root.quads + " / Material Switches: " + root.materialSwitches 
                }
                Text { 
                    color: "white"; 
                    visible: root.expanded;  
                    text: "\tMesh Parts Rendered Opaque: " + root.meshOpaque + 
                        " / Translucent: " + root.meshTranslucent; 
                }
                Text { 
                    color: "white"; 
                    visible: root.expanded;  
                    text: "\tOpaque considered: " + root.opaqueConsidered + 
                        " / Out of view: " + root.opaqueOutOfView + " / Too small: " + root.opaqueTooSmall; 
                }
                Text { 
                    color: "white"; 
                    visible: !root.expanded 
                    text: "Octree Elements Server: "; 
                }
            }
        }
    }

    Connections {
        target: root.parent
        onWidthChanged: {
            root.x = root.parent.width - root.width;
        }
    }
}

