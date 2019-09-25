//
//  ControlsTable.qml
//
//  Created by Zach Fox on 2019-08-16
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Layouts 1.3
import stylesUit 1.0 as HifiStylesUit

Item {
    id: controlsTableRoot
    property int column1Width: 80
    property int column2Width: width - column1Width
    property int rowHeight: 31
    property int rowPadding: 8
    property int mainTextSize: 18
    property int subTextSize: 14
    property color separatorColor: "#CCCCCC"
    Layout.preferredHeight: controlsTableColumnLayout.height
        
    // Top separator
    Rectangle {
        anchors.top: controlsTableColumnLayout.top
        anchors.left: controlsTableColumnLayout.left
        width: parent.width
        height: 1
        color: controlsTableRoot.separatorColor
    }

    // Right separator
    Rectangle {
        anchors.top: controlsTableColumnLayout.top
        anchors.right: controlsTableColumnLayout.right
        width: 1
        height: controlsTableColumnLayout.height
        color: controlsTableRoot.separatorColor
    }

    // Bottom separator
    Rectangle {
        anchors.top: controlsTableColumnLayout.top
        anchors.topMargin: controlsTableRoot.height
        anchors.left: controlsTableColumnLayout.left
        width: parent.width
        height: 1
        color: controlsTableRoot.separatorColor
    }

    // Left separator
    Rectangle {
        anchors.top: controlsTableColumnLayout.top
        anchors.left: controlsTableColumnLayout.left
        width: 1
        height: controlsTableColumnLayout.height
        color: controlsTableRoot.separatorColor
    }
    
    ColumnLayout {
        id: controlsTableColumnLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 0

        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                Image {
                    source: "images/rightClick.svg"
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    mipmap: true
                    fillMode: Image.PreserveAspectFit
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            Row {
                width: controlsTableRoot.column2Width
                height: parent.height
                spacing: controlsTableRoot.rowPadding

                HifiStylesUit.GraphikRegular {
                    id: cameraText
                    text: "Camera"
                    width: paintedWidth
                    height: parent.height
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                HifiStylesUit.GraphikRegular {
                    text: "Right-click and drag to look around"
                    width: parent.width - cameraText.width - parent.spacing - controlsTableRoot.rowPadding
                    height: parent.height
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    size: controlsTableRoot.subTextSize
                    color: Qt.rgba(255, 255, 255, 0.5)
                }
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        



        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "W / ↑"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            HifiStylesUit.GraphikRegular {
                text: "Walk Forward"
                width: controlsTableRoot.column2Width - controlsTableRoot.rowPadding * 2
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                size: controlsTableRoot.mainTextSize
                color: simplifiedUI.colors.text.white
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "S / ↓"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            HifiStylesUit.GraphikRegular {
                text: "Walk Backward"
                width: controlsTableRoot.column2Width - controlsTableRoot.rowPadding * 2
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                size: controlsTableRoot.mainTextSize
                color: simplifiedUI.colors.text.white
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "A / ←"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            HifiStylesUit.GraphikRegular {
                text: "Turn Left"
                width: controlsTableRoot.column2Width - controlsTableRoot.rowPadding * 2
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                size: controlsTableRoot.mainTextSize
                color: simplifiedUI.colors.text.white
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "D / →"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            HifiStylesUit.GraphikRegular {
                text: "Turn Right"
                width: controlsTableRoot.column2Width - controlsTableRoot.rowPadding * 2
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                size: controlsTableRoot.mainTextSize
                color: simplifiedUI.colors.text.white
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "Q"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            HifiStylesUit.GraphikRegular {
                text: "Sidestep Left"
                width: controlsTableRoot.column2Width - controlsTableRoot.rowPadding * 2
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                size: controlsTableRoot.mainTextSize
                color: simplifiedUI.colors.text.white
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "E"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            HifiStylesUit.GraphikRegular {
                text: "Sidestep Right"
                width: controlsTableRoot.column2Width - controlsTableRoot.rowPadding * 2
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                size: controlsTableRoot.mainTextSize
                color: simplifiedUI.colors.text.white
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "Shift"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            Row {
                width: controlsTableRoot.column2Width
                height: parent.height
                spacing: controlsTableRoot.rowPadding

                HifiStylesUit.GraphikRegular {
                    id: runText
                    text: "Hold + Direction to Run"
                    width: paintedWidth
                    height: parent.height
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                HifiStylesUit.GraphikRegular {
                    text: "Example: Shift + W"
                    width: parent.width - runText.width - parent.spacing - controlsTableRoot.rowPadding
                    height: parent.height
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    size: controlsTableRoot.subTextSize
                    color: Qt.rgba(255, 255, 255, 0.5)
                }
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "Space"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            Row {
                width: controlsTableRoot.column2Width
                height: parent.height
                spacing: controlsTableRoot.rowPadding

                HifiStylesUit.GraphikRegular {
                    id: jumpText
                    text: "Jump / Stand Up"
                    width: paintedWidth
                    height: parent.height
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                HifiStylesUit.GraphikRegular {
                    text: "Stand Up only while seated"
                    width: parent.width - jumpText.width - parent.spacing - controlsTableRoot.rowPadding
                    height: parent.height
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    size: controlsTableRoot.subTextSize
                    color: Qt.rgba(255, 255, 255, 0.5)
                }
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "1"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            HifiStylesUit.GraphikRegular {
                text: "1st Person View"
                width: controlsTableRoot.column2Width - controlsTableRoot.rowPadding * 2
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                size: controlsTableRoot.mainTextSize
                color: simplifiedUI.colors.text.white
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "2"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            Row {
                width: controlsTableRoot.column2Width
                height: parent.height
                spacing: controlsTableRoot.rowPadding

                HifiStylesUit.GraphikRegular {
                    id: selfieText
                    text: "Selfie"
                    width: paintedWidth
                    height: parent.height
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                HifiStylesUit.GraphikRegular {
                    text: "Look at self"
                    width: parent.width - selfieText.width - parent.spacing - controlsTableRoot.rowPadding
                    height: parent.height
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    size: controlsTableRoot.subTextSize
                    color: Qt.rgba(255, 255, 255, 0.5)
                }
            }
        }

        // Bottom separator
        Rectangle {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 1
            color: controlsTableRoot.separatorColor
        }
        

        

        
        Row {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: controlsTableRoot.rowHeight

            Item {
                width: controlsTableRoot.column1Width
                height: parent.height

                HifiStylesUit.GraphikRegular {
                    text: "3"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    size: controlsTableRoot.mainTextSize
                    color: simplifiedUI.colors.text.white
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: controlsTableRoot.separatorColor
                    anchors.right: parent.right
                    anchors.top: parent.top
                }
            }

            // Spacer
            Item {
                width: controlsTableRoot.rowPadding
                height: parent.height
            }

            HifiStylesUit.GraphikRegular {
                text: "3rd Person View"
                width: controlsTableRoot.column2Width - controlsTableRoot.rowPadding * 2
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                size: controlsTableRoot.mainTextSize
                color: simplifiedUI.colors.text.white
            }
        }
    }
}