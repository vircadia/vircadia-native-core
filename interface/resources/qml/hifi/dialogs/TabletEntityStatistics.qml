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

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Elements on Servers:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: elementsOnServerLabel
                size: 20
                text: OctreeStats.serverElements
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Local Elements:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: localElementsLabel
                size: 20
                text: OctreeStats.localElements
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Elements Memory:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: elementsMemoryLabel
                size: 20
                text: OctreeStats.localElementsMemory
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Sending Mode:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: sendingModeLabel
                size: 20
                text: OctreeStats.sendingMode
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Incoming Entity Packets:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: incomingEntityPacketsLabel
                size: 20
                text: OctreeStats.processedPackets
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Processed Packets Elements:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: processedPacketsElementsLabel
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Processed Packets Entities:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: processedPacketsEntitiesLabel
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Processed Packets Timing:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: processedPacketsTimingLabel
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Outbound Entity Packets:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: outboundEntityPacketsLabel
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Entity Update Time:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: entityUpdateTimeLabel
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                colorScheme: root.colorScheme
            }

            HifiControls.Label {
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Entity Updates:")
                colorScheme: root.colorScheme
            }
            HifiControls.Label {
                id: entityUpdatesLabel
                size: 20
                anchors.left: parent.left
                anchors.right: parent.right
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

                    HifiControls.Label {
                        size: 20
                        width: parent.width
                        text: qsTr("Entity Server ") + (index+1) + ":"
                        colorScheme: root.colorScheme
                    }
                    HifiControls.Label {
                        id: entityServer1Label
                        size: 20
                        width: parent.width
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
