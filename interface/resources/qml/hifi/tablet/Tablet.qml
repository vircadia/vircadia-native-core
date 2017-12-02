import QtQuick 2.7
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import "../../styles-uit"
import "../audio" as HifiAudio

Item {
    id: tablet
    objectName: "tablet"
    property int rowIndex: 6 // by default
    property int columnIndex: 1 // point to 'go to location'
    property int count: 0

    // used to look up a button by its uuid
    function findButtonIndex(uuid) {
        if (!uuid) {
            return { page: -1, index: -1};
        }
        for (var gridIndex = 0;  gridIndex < swipeView.count; gridIndex++) {
            var grid = swipeView.itemAt(gridIndex).children[0]
            for (var i in grid.children) {
                var child = grid.children[i];
                if (child.uuid === uuid) {
                    return { page: gridIndex, index: i};
                }
            }
        }
        return { page: -1, index: -1};
    }

    function sortButtons(gridIndex) {
        var children = [];
        var grid = swipeView.itemAt(gridIndex).children[0]

        for (var i = 0; i < grid.children.length; i++) {
            children[i] = grid.children[i];
        }

        children.sort(function (a, b) {
            if (a.sortOrder === b.sortOrder) {
                // subsort by stableOrder, because JS sort is not stable in qml.
                return a.stableOrder - b.stableOrder;
            } else {
                return a.sortOrder - b.sortOrder;
            }
        });

        grid.children = children;
    }

    function doAddButton(grid, gridIndex, component, properties) {
        var button = component.createObject(grid.children[0]);

        // copy all properites to button
        var keys = Object.keys(properties).forEach(function (key) {
            button[key] = properties[key];
        });

        // pass a reference to the tabletRoot object to the button.
        if (tabletRoot) {
            button.tabletRoot = tabletRoot;
        } else {
            button.tabletRoot = parent.parent;
        }
        button.flickable = swipeView.contentItem

        sortButtons(gridIndex);

        tablet.count++
        return button;
    }

    Component {
        id: pageComponent
        Item {
            visible: SwipeView.isCurrentItem
            Grid {
                anchors {
                    fill: parent
                    topMargin: 20
                    leftMargin: 30
                    rightMargin: 30
                    bottomMargin: 0
                }
                rows: 4; columns: 3; rowSpacing: 16; columnSpacing: 16;
            }
        }
    }

    // called by C++ code when a button should be added to the tablet
    function addButtonProxy(properties) {
        var component = Qt.createComponent("TabletButton.qml");
        var grid = swipeView.itemAt(swipeView.count - 1)
        if (grid === null || grid.children[0].children.length === 12) {
            grid = pageComponent.createObject(swipeView)
            swipeView.addItem(grid)
        }
        return doAddButton(grid, swipeView.count - 1, component, properties)
    }

    // called by C++ code when a button should be removed from the tablet
    function removeButtonProxy(properties) {
        var index = findButtonIndex(properties.uuid);
        if (index.index < 0) {
            console.log("Warning: Tablet.qml could not find button with uuid = " + properties.uuid);
        } else {
            var grid = swipeView.itemAt(index.page).children[0]
            grid.children[index.index].destroy();
            tablet.count--
        }
    }

    Rectangle {
        id: bgTopBar
        height: 90

        anchors {
            top: parent.top
            topMargin: 0
            left: parent.left
            leftMargin: 0
            right: parent.right
            rightMargin: 0
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
            currentIndex: pageIndicator.currentIndex
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
                    opacity: index === pageIndicator.currentIndex ? 0.95 : pressed ? 0.7 : 0.45
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
                MouseArea {
                    anchors.fill: parent
                    enabled: false //disabled by default
                    onClicked: {
                        if (index !== swipeView.currentIndex) {
                            swipeView.currentIndex = index
                        }
                    }
                }
            }

            interactive: false
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            count: swipeView.count
        }
    }

    function getPage(row, column) {
        var pageIndex = Math.floor((row + column) / 12)
        var index = (row + column) % 12
        var page = swipeView.itemAt(pageIndex).children[0]
        return { page: page, index: index, pageIndex: pageIndex }
    }

    function setCurrentItemState(state) {
        var index = rowIndex + columnIndex;
        if (index >= 0 && index <= count ) {
            var grid = getPage(rowIndex, columnIndex)
            grid.page.children[grid.index].state = state;
            swipeView.currentIndex = grid.pageIndex
        }
    }

    function nextItem() {
        setCurrentItemState("base state");
        var nextColumnIndex = (columnIndex + 3 + 1) % 3;
        var nextIndex = rowIndex + nextColumnIndex;
        if(nextIndex < count) {
            columnIndex = nextColumnIndex;
        };
        setCurrentItemState("hover state");
    }

    function previousItem() {
        setCurrentItemState("base state");
        var prevIndex = (columnIndex + 3 - 1) % 3;
        if((rowIndex + prevIndex) < count){
            columnIndex = prevIndex;
        }
        setCurrentItemState("hover state");
    }

    function upItem() {
        setCurrentItemState("base state");
        rowIndex = rowIndex - 3;
        if (rowIndex < 0 ) {
            rowIndex =  (count - (count % 3));
            var index = rowIndex + columnIndex;
            if(index  > count) {
                rowIndex = rowIndex - 3;
            }
        }
        setCurrentItemState("hover state");
    }

    function downItem() {
        setCurrentItemState("base state");
        rowIndex = rowIndex + 3;
        var index = rowIndex + columnIndex;
        if (index  > count ) {
            rowIndex = 0;
        }
        setCurrentItemState("hover state");
    }

    function selectItem() {
        var grid = getPage(rowIndex, columnIndex)
        grid.page.children[grid.index].clicked();
        if (tabletRoot) {
            tabletRoot.playButtonClickSound();
        }
    }

    Keys.onRightPressed: nextItem();
    Keys.onLeftPressed: previousItem();
    Keys.onDownPressed: downItem();
    Keys.onUpPressed: upItem();
    Keys.onReturnPressed: selectItem();
}
