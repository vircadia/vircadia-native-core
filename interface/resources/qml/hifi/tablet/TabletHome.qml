import QtQuick 2.7
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import TabletScriptingInterface 1.0

import "."
import stylesUit 1.0
import "../audio" as HifiAudio

Item {
    id: tablet
    objectName: "tablet"
    property var tabletProxy: Tablet.getTablet("com.highfidelity.interface.tablet.system");

    property var currentGridItems: null

    focus: true

    Rectangle {
        id: bgTopBar
        height: 90

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
            }

            GradientStop {
                position: 1
                color: "#1e1e1e"
            }
        }

        HifiAudio.MicBar {
            anchors {
                left: parent.left
                leftMargin: 30
                verticalCenter: parent.verticalCenter
            }
        }

        Item {
            width: 150
            height: 50
            anchors.right: parent.right
            anchors.rightMargin: 30
            anchors.verticalCenter: parent.verticalCenter

            ColumnLayout {
                anchors.fill: parent

                RalewaySemiBold {
                    text: Account.loggedIn ? qsTr("Log out") : qsTr("Log in")
                    horizontalAlignment: Text.AlignRight
                    anchors.right: parent.right
                    font.pixelSize: 20
                    color: "#afafaf"
                }

                RalewaySemiBold {
                    visible: Account.loggedIn
                    height: Account.loggedIn ? parent.height/2 - parent.spacing/2 : 0
                    text: Account.loggedIn ? "[" + tabletRoot.usernameShort + "]" : ""
                    horizontalAlignment: Text.AlignRight
                    anchors.right: parent.right
                    font.pixelSize: 20
                    color: "#afafaf"
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!Account.loggedIn) {
                        DialogsManager.showLoginDialog()
                    } else {
                        Account.logOut()
                    }
                }
            }
        }
    }

    Rectangle {
        id: bgMain
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
            }

            GradientStop {
                position: 1
                color: "#0f212e"
            }
        }
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: bgTopBar.bottom

        SwipeView {
            id: swipeView
            clip: false
            currentIndex: -1
            property int previousIndex: -1
            Repeater {
                id: pageRepeater
                model: Math.ceil(tabletProxy.buttons.rowCount() / TabletEnums.ButtonsOnPage)
                onItemAdded: {
                    item.proxyModel.sourceModel = tabletProxy.buttons;
                    item.proxyModel.pageIndex = index;
                }

                delegate: Item {
                    id: page
                    property TabletButtonsProxyModel proxyModel: TabletButtonsProxyModel {}

                    GridView {
                        id: gridView
                        flickableDirection: Flickable.AutoFlickIfNeeded
                        keyNavigationEnabled: false
                        highlightFollowsCurrentItem: false
                        interactive: false

                        property int previousGridIndex: -1

                        // true if any of the buttons contains mouse
                        property bool containsMouse: false

                        anchors {
                            fill: parent
                            topMargin: 20
                            leftMargin: 30
                            rightMargin: 30
                            bottomMargin: 0
                        }

                        onCurrentIndexChanged: {
                            previousGridIndex = currentIndex
                        }

                        onMovementStarted: {
                            if (currentIndex < 0 || gridView.currentItem === undefined || gridView.contentItem.children.length - 1 < currentIndex) {
                                return;
                            }
                            var button = gridView.contentItem.children[currentIndex].children[0];
                            if (button.isActive) {
                                button.state = "active state";
                            } else {
                                button.state = "base state";
                            }
                        }

                        cellWidth: width/3
                        cellHeight: cellWidth
                        flow: GridView.LeftToRight
                        model: page.proxyModel

                        delegate: Control {
                            id: wrapper
                            width: gridView.cellWidth
                            height: gridView.cellHeight

                            hoverEnabled: true

                            property bool containsMouse: gridView.containsMouse
                            onHoveredChanged: {
                                if (hovered && !gridView.containsMouse) {
                                    gridView.containsMouse = true
                                } else {
                                    gridView.containsMouse = false
                                }
                            }

                            property var proxy: modelData

                            TabletButton {
                                id: tabletButton

                                // Temporarily disable magnification
                                // scale: wrapper.hovered ? 1.25 : wrapper.containsMouse ? 0.75 : 1.0
                                // Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.Linear } }

                                anchors.centerIn: parent
                                gridView: wrapper.GridView.view
                                buttonIndex: page.proxyModel.buttonIndex(uuid);
                                flickable: swipeView.contentItem;
                                onClicked: modelData.clicked()
                            }

                            Connections {
                                target: modelData;
                                onPropertiesChanged:  {
                                    updateProperties();
                                }
                            }

                            Component.onCompleted: updateProperties()

                            function updateProperties() {
                                var keys = Object.keys(modelData.properties).forEach(function (key) {
                                    if (tabletButton[key] !== modelData.properties[key]) {
                                        tabletButton[key] = modelData.properties[key];
                                    }
                                });
                            }
                        }
                    }
                }
            }

            onCurrentIndexChanged: {
                if (swipeView.currentIndex < 0
                        || swipeView.itemAt(swipeView.currentIndex) === null
                        || swipeView.itemAt(swipeView.currentIndex).children[0] === null) {
                    return;
                }

                currentGridItems = swipeView.itemAt(swipeView.currentIndex).children[0];

                currentGridItems.currentIndex = (previousIndex > swipeView.currentIndex ? currentGridItems.count - 1 : 0);
                previousIndex = currentIndex;
            }

            hoverEnabled: true
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: pageIndicator.top
            }
        }

        PageIndicator {
            id: pageIndicator
            currentIndex: swipeView.currentIndex
            visible: swipeView.count > 1

            delegate: Item {
                width: 15
                height: 15

                Rectangle {
                    property bool isHovered: false
                    anchors.centerIn: parent
                    opacity: index === pageIndicator.currentIndex || isHovered ? 0.95 : 0.45
                    implicitWidth: index === pageIndicator.currentIndex || isHovered ? 15 : 10
                    implicitHeight: implicitWidth
                    radius: width/2
                    color: isHovered && index !== pageIndicator.currentIndex ? "#1fc6a6" : "white"
                    Behavior on opacity {
                        OpacityAnimator {
                            duration: 100
                        }
                    }

                    MouseArea {
                        anchors.centerIn: parent
                        width: 20
                        height: 30 // Make it easier to target with laser.
                        hoverEnabled: true
                        enabled: true
                        onEntered: parent.isHovered = true;
                        onExited: parent.isHovered = false;
                        onClicked: swipeView.currentIndex = index;
                    }
                }
            }

            interactive: false
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            count: swipeView.count
        }
    }

    Component.onCompleted: {
        focus = true;
        forceActiveFocus();
    }

    Keys.onRightPressed: {
        if (!currentGridItems) {
            return;
        }

        var index = currentGridItems.currentIndex;
        currentGridItems.moveCurrentIndexRight();
        if (index === currentGridItems.count - 1 && index === currentGridItems.currentIndex) {
            if (swipeView.currentIndex < swipeView.count - 1) {
                swipeView.incrementCurrentIndex();
            }
        }
    }

    Keys.onLeftPressed: {
        if (!currentGridItems) {
            return;
        }

        var index = currentGridItems.currentIndex;
        currentGridItems.moveCurrentIndexLeft();
        if (index === 0 && index === currentGridItems.currentIndex) {
            if (swipeView.currentIndex > 0) {
                swipeView.decrementCurrentIndex();
            }
        }
    }
    Keys.onDownPressed: currentGridItems.moveCurrentIndexDown();
    Keys.onUpPressed: currentGridItems.moveCurrentIndexUp();
    Keys.onReturnPressed: {
        if (currentGridItems.currentItem) {
            currentGridItems.currentItem.proxy.clicked();
            if (tabletRoot) {
                tabletRoot.playButtonClickSound();
            }
        }
    }
}
