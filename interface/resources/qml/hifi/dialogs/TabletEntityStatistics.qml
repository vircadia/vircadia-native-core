//
//  TabletEntityStatistics.qml
//
//  Created by Vlad Stelmahovsky on  3/11/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../../styles-uit"
import "../../controls-uit" as HifiControls
import "../../windows"

Rectangle {
    id: root
    objectName: "EntityStatistics"

    property var eventBridge;
    signal sendToScript(var message);
    property bool isHMD: false

    color: hifi.colors.baseGray

    property int colorScheme: hifi.colorSchemes.dark

    HifiConstants { id: hifi }

    Component.onCompleted: {
        OctreeStats.startUpdates()
    }

    Component.onDestruction: {
        OctreeStats.stopUpdates()
    }

    Flickable {
        id: scrollView
        width: parent.width
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: hifi.dimensions.tabletMenuHeader
        contentWidth: column.implicitWidth
        contentHeight: column.implicitHeight

        Column {
            id: column
            anchors.margins: 10
            anchors.left: parent.left
            anchors.right: parent.right
            y: hifi.dimensions.tabletMenuHeader //-bgNavBar
            spacing: 20

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Elements on Servers:")
                text: OctreeStats.serverElements
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Local Elements:")
                text: OctreeStats.localElements
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Elements Memory:")
                text: OctreeStats.localElementsMemory
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Sending Mode:")
                text: OctreeStats.sendingMode
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Incoming Entity Packets:")
                text: OctreeStats.processedPackets
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Processed Packets Elements:")
                text: OctreeStats.processedPacketsElements
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Processed Packets Entities:")
                text: OctreeStats.processedPacketsEntities
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Processed Packets Timing:")
                text: OctreeStats.processedPacketsTiming
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Outbound Entity Packets:")
                text: OctreeStats.outboundEditPackets
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Entity Update Time:")
                text: OctreeStats.entityUpdateTime
                colorScheme: root.colorScheme
            }

            TabletEntityStatisticsItem {
                anchors.left: parent.left
                anchors.right: parent.right
                titleText: qsTr("Entity Updates:")
                text: OctreeStats.entityUpdates
                colorScheme: root.colorScheme
            }

            Repeater {
                model: OctreeStats.serversNum

                delegate: Column {
                    id: serverColumn
                    width: scrollView.width - 10
                    x: 5
                    spacing: 5

                    state: "less"

                    TabletEntityStatisticsItem {
                        width: parent.width
                        titleText: qsTr("Entity Server ") + (index+1) + ":"
                        colorScheme: root.colorScheme
                    }

                    Row {
                        id: buttonsRow
                        width: parent.width
                        height: 30
                        spacing: 10

                        HifiControls.Button {
                            id: moreButton
                            color: hifi.buttons.blue
                            colorScheme: root.colorScheme
                            width: parent.width / 2 - 10
                            height: 30
                            onClicked: {
                                if (serverColumn.state === "less") {
                                    serverColumn.state = "more"
                                } else if (serverColumn.state === "more") {
                                    serverColumn.state = "most"
                                } else {
                                    serverColumn.state = "less"
                                }
                            }
                        }
                        HifiControls.Button {
                            id: mostButton
                            color: hifi.buttons.blue
                            colorScheme: root.colorScheme
                            height: 30
                            width: parent.width / 2 - 10
                            onClicked: {
                                if (serverColumn.state === "less") {
                                    serverColumn.state = "most"
                                } else if (serverColumn.state === "more") {
                                    serverColumn.state = "less"
                                } else {
                                    serverColumn.state = "more"
                                }
                            }

                        }
                    }
                    states: [
                        State {
                            name: "less"
                            PropertyChanges { target: moreButton; text: qsTr("more..."); }
                            PropertyChanges { target: mostButton; text: qsTr("most..."); }
                        },
                        State {
                            name: "more"
                            PropertyChanges { target: moreButton; text: qsTr("most..."); }
                            PropertyChanges { target: mostButton; text: qsTr("less..."); }
                        },
                        State {
                            name: "most"
                            PropertyChanges { target: moreButton; text: qsTr("less..."); }
                            PropertyChanges { target: mostButton; text: qsTr("least..."); }
                        }
                    ]
                } //servers column
            } //repeater
        } //column
    } //flickable
}
