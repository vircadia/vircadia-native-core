import QtQuick 2.7
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import "."
import "../../styles-uit"
import "../audio" as HifiAudio

Item {
    id: tablet
    objectName: "tablet"
    property var tabletProxy: Tablet.getTablet("com.highfidelity.interface.tablet.system");

    property int rowIndex: 6 // by default
    property int columnIndex: 1 // point to 'go to location'
    readonly property int buttonsOnPage: 12
    readonly property int buttonsRowsOnPage: 4
    readonly property int buttonsColumnsOnPage: 3

    property var currentGridItems: null
    property var gridPages: [];

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
                model: Math.ceil(tabletProxy.buttons.rowCount() / buttonsOnPage)
                onCountChanged: console.warn("repeat count", count, tabletProxy.buttons.rowCount())
                onItemAdded: {
                    item.pageIndex = index
                }

                delegate: Item {
                    id: page
                    property int pageIndex
                    onPageIndexChanged: console.warn("page index", pageIndex)
                    Grid {
                        anchors {
                            fill: parent
                            topMargin: 20
                            leftMargin: 30
                            rightMargin: 30
                            bottomMargin: 0
                        }
                        rows: 4; columns: 3; rowSpacing: 16; columnSpacing: 16;
                        Repeater {
                            id: buttonsRepeater
                            model: tabletProxy.buttons.rowCount() - (((page.pageIndex + 1) *  buttonsOnPage) % buttonsOnPage)
                            delegate: Item {
                                id: wrapper
                                width: 129
                                height: 129
                                property var proxy: modelData

                                TabletButton {
                                    id: tabletButton
                                    anchors.centerIn: parent
                                    onClicked: modelData.clicked()
                                    state: wrapper.GridView.isCurrentItem ? "hover state" : "base state"
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
                        Component.onCompleted: {
//                            tabletProxy.buttonsOnPage.setCurrentPage(page.index);
//                            buttonsRepeater.model = tabletProxy.buttonsOnPage;
//                            console.warn("buttons on page:", page.index, tabletProxy.buttonsOnPage.rowCount())
                        }
                    }
                }
            }

            onCurrentIndexChanged: {
//                if (swipeView.currentIndex < 0
//                        || swipeView.itemAt(swipeView.currentIndex) === null
//                        || swipeView.itemAt(swipeView.currentIndex).children[0] === null
//                        || swipeView.itemAt(swipeView.currentIndex).children[0].children === null) {
//                    return;
//                }

//                currentGridItems = swipeView.itemAt(swipeView.currentIndex).children[0].children;

//                var row = rowIndex < 0 ? 0 : rowIndex;
//                var column = previousIndex > swipeView.currentIndex ? buttonsColumnsOnPage - 1 : 0;
//                var index = row * buttonsColumnsOnPage + column;
//                if (index < 0 || index >= currentGridItems.length) {
//                    column = 0;
//                    row = 0;
//                }
//                rowIndex = row;
//                columnIndex = column;
//                setCurrentItemState("hover state");
//                previousIndex = currentIndex;
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

            delegate: Item {
                width: 15
                height: 15

                Rectangle {
                    anchors.centerIn: parent
                    opacity: index === pageIndicator.currentIndex ? 0.95 : 0.45
                    implicitWidth: index === pageIndicator.currentIndex ? 15 : 10
                    implicitHeight: implicitWidth
                    radius: width/2
                    color: "white"
                    Behavior on opacity {
                        OpacityAnimator {
                            duration: 100
                        }
                    }
                }
            }

            interactive: false
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            count: swipeView.count
        }
//        GridView {
//            id: flickable
//            anchors.top: parent.top
//            anchors.topMargin: 15
//            anchors.bottom: parent.bottom
//            anchors.horizontalCenter: parent.horizontalCenter
//            width: cellWidth * 3
//            cellHeight: 145
//            cellWidth: 145
//            flow: GridView.LeftToRight
//            model: tabletProxy.buttons
//            delegate: Item {
//                width: flickable.cellWidth
//                height: flickable.cellHeight
//                property var proxy: modelData

//                TabletButton {
//                    id: tabletButton
//                    anchors.centerIn: parent
//                    onClicked: modelData.clicked()
//                    state: wrapper.GridView.isCurrentItem ? "hover state" : "base state"
//                }

//                Connections {
//                    target: modelData;
//                    onPropertiesChanged:  {
//                        updateProperties();
//                    }
//                }

//                Component.onCompleted: updateProperties()

//                function updateProperties() {
//                    var keys = Object.keys(modelData.properties).forEach(function (key) {
//                        if (tabletButton[key] !== modelData.properties[key]) {
//                            tabletButton[key] = modelData.properties[key];
//                        }
//                    });
//                }
//            }
//        }
    }

    function setCurrentItemState(state) {
        var buttonIndex = rowIndex * buttonsColumnsOnPage + columnIndex;
        if (currentGridItems !== null && buttonIndex >= 0 && buttonIndex < currentGridItems.length) {
            if (currentGridItems[buttonIndex].isActive) {
                currentGridItems[buttonIndex].state = "active state";
            } else {
                currentGridItems[buttonIndex].state = state;
            }
        }
    }

//    Keys.onRightPressed: flickable.moveCurrentIndexRight();
//    Keys.onLeftPressed: flickable.moveCurrentIndexLeft();
//    Keys.onDownPressed: flickable.moveCurrentIndexDown();
//    Keys.onUpPressed: flickable.moveCurrentIndexUp();
//    Keys.onReturnPressed: {
//        if (flickable.currentItem) {
//            flickable.currentItem.proxy.clicked();
//            if (tabletRoot) {
//                tabletRoot.playButtonClickSound();
//            }
//        }
//    }
}
