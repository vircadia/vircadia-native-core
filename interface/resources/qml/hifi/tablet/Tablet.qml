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
    readonly property int buttonsOnPage: 12
    readonly property int buttonsRowsOnPage: 4
    readonly property int buttonsColumnsOnPage: 3

    focus: true

    Timer {
        id: gridsRecreateTimer
        interval: 200
        repeat: false
        onTriggered: {
            doRecreateGrids()
        }
    }

    //fake invisible item for initial buttons creations
    Item {
        id: fakeParent
        visible: parent
    }

    function doRecreateGrids() {
        var tabletProxy = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        var tabletButtons = tabletProxy.getButtonsList();
        for (var i = 0; i < swipeView.count; i++) {
            var gridOuter = swipeView.itemAt(i);
            gridOuter.destroy();
            swipeView.removeItem(i);
        }
        sortButtons(tabletButtons);

        for (var buttonIndex = 0; buttonIndex < tabletButtons.length; buttonIndex++) {
            var button = tabletButtons[buttonIndex]
            if (button !== null) {
                var grid = swipeView.itemAt(swipeView.count - 1);
                if (grid === null || grid.children[0].children.length === buttonsOnPage) {
                    grid = pageComponent.createObject(swipeView);
                }
                button.row = Math.floor(grid.children[0].children.length / buttonsColumnsOnPage);
                button.column = grid.children[0].children.length % buttonsColumnsOnPage;
                //reparent to actual grid
                button.parent = grid.children[0];
                grid.children[0].children.push(button);
            }
        }
    }

    // used to look up a button by its uuid
    function findButtonIndex(uuid) {
        if (!uuid) {
            return -1;
        }
        var tabletProxy = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        var tabletButtons = tabletProxy.getButtonsList();
        for (var i in tabletButtons) {
            var child = tabletButtons[i];
            if (child.uuid === uuid) {
                return i;
            }
        }

        return -1;
    }

    function sortButtons(tabletButtons) {
        tabletButtons.sort(function (a, b) {
            if (a === null || b === null) {
                return 0;
            }
            if (a.sortOrder === b.sortOrder) {
                // subsort by stableOrder, because JS sort is not stable in qml.
                return a.stableOrder - b.stableOrder;
            } else {
                return a.sortOrder - b.sortOrder;
            }
        });
    }

    function doAddButton(component, properties) {
        var button = component.createObject(fakeParent);

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
        button.flickable = swipeView.contentItem;
        tablet.count++
        gridsRecreateTimer.restart();

        return button;
    }

    Component {
        id: pageComponent
        Item {
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
        return doAddButton(component, properties);
    }

    // called by C++ code when a button should be removed from the tablet
    function removeButtonProxy(properties) {
        var index = findButtonIndex(properties.uuid);
        if (index < 0) {
            console.log("Warning: Tablet.qml could not find button with uuid = " + properties.uuid);
        } else {
            tablet.count--;
            //redraw grids
            gridsRecreateTimer.restart();
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
            onCurrentIndexChanged: {
                rowIndex = 0;
                columnIndex = 0;
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
            }

            interactive: false
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            count: swipeView.count
        }
    }

    function setCurrentItemState(state) {
        var index = rowIndex*buttonsColumnsOnPage + columnIndex;
        var currentGridItems = swipeView.currentItem.children[0].children;
        if (currentGridItems !== null && index >= 0 && index < currentGridItems.length) {
            if (currentGridItems[index].isActive) {
                currentGridItems[index].state = "active state";
            } else {
                currentGridItems[index].state = state;
            }
        }
    }

    function previousPage() {
        setCurrentItemState("base state");
        var currentPage = swipeView.currentIndex;
        currentPage = currentPage - 1;
        if (currentPage < 0) {
            currentPage = swipeView.count - 1;
        }
        swipeView.currentIndex = currentPage;
    }

    function nextPage() {
        setCurrentItemState("base state");
        var currentPage = swipeView.currentIndex;
        currentPage = currentPage + 1;
        if (currentPage >= swipeView.count) {
            currentPage = 0;
        }
        swipeView.currentIndex = currentPage;
    }

    function nextItem() {
        setCurrentItemState("base state");
        var currentGridItems = swipeView.currentItem.children[0].children;
        var nextColumnIndex = columnIndex + 1;
        var index = rowIndex*buttonsColumnsOnPage + nextColumnIndex;
        if(index >= currentGridItems.length || nextColumnIndex >= buttonsColumnsOnPage) {
            nextColumnIndex = 0;
        }
        columnIndex = nextColumnIndex;
        setCurrentItemState("hover state");
    }

    function previousItem() {
        setCurrentItemState("base state");
        var column = columnIndex;
        column = column - 1;

        if (column < 0 ) {
            column =  buttonsColumnsOnPage - 1;
            var index = rowIndex*buttonsColumnsOnPage + column;
            var currentGridItems = swipeView.currentItem.children[0].children;
            while(index >= currentGridItems.length) {
                column = column - 1;
                index = rowIndex*buttonsColumnsOnPage + column;
            }
        }
        columnIndex = column;
        setCurrentItemState("hover state");
    }

    function upItem() {
        setCurrentItemState("base state");
        var row = rowIndex;
        row = row - 1;

        if (row < 0 ) {
            row =  buttonsRowsOnPage - 1;
            var index = row*buttonsColumnsOnPage + columnIndex;
            var currentGridItems = swipeView.currentItem.children[0].children;
            while(index >= currentGridItems.length) {
                row = row - 1;
                index = row*buttonsColumnsOnPage + columnIndex;
            }
        }
        rowIndex = row;
        setCurrentItemState("hover state");
    }

    function downItem() {
        setCurrentItemState("base state");
        rowIndex = rowIndex + 1;
        var currentGridItems = swipeView.currentItem.children[0].children;
        var index = rowIndex*buttonsColumnsOnPage + columnIndex;
        if (index >= currentGridItems.length) {
            rowIndex = 0;
        }
        setCurrentItemState("hover state");
    }

    function selectItem() {
        var index = rowIndex*buttonsColumnsOnPage + columnIndex;
        var currentGridItems = swipeView.currentItem.children[0].children;
        if (currentGridItems !== null && index >= 0 && index < currentGridItems.length) {
            currentGridItems[index].clicked();
            if (tabletRoot) {
                tabletRoot.playButtonClickSound();
            }
        }
    }

    Keys.onRightPressed: {
        if (event.modifiers & Qt.ShiftModifier) {
            nextPage();
        } else {
            nextItem();
        }
    }
    Keys.onLeftPressed: {
        if (event.modifiers & Qt.ShiftModifier) {
            previousPage();
        } else {
            previousItem();
        }
    }
    Keys.onDownPressed: {
        if (event.modifiers & Qt.ShiftModifier) {
            nextPage();
        } else {
            downItem();
        }
    }
    Keys.onUpPressed: {
        if (event.modifiers & Qt.ShiftModifier) {
            previousPage();
        } else {
            upItem();
        }
    }
    Keys.onReturnPressed: selectItem();
}
