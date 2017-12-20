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

    property var currentGridItems: null
    property var gridPages: [];

    focus: true

    Timer {
        id: gridsRecreateTimer
        interval: 1
        repeat: false
        onTriggered: {
            doRecreateGrids()
        }
    }

    //fake invisible item for initial buttons creations.
    //Also used as a holder for buttons list
    Item {
        id: fakeParent
        visible: false
    }

    function doRecreateGrids() {

        if (fakeParent.resources.length <= 0) {
            console.warn("Tablet buttons list is empty", fakeParent.resources.length);
            return;
        }

        //copy buttons list to JS array to be able to sort it
        var tabletButtons = [];
        for (var btnIndex = 0; btnIndex < fakeParent.resources.length; btnIndex++) {
            tabletButtons.push(fakeParent.resources[btnIndex]);
        }

        for (var i = swipeView.count - 1; i >= 0; i--) {
            swipeView.removeItem(i);
        }
        gridPages = [];

        swipeView.setCurrentIndex(-1);
        sortButtons(tabletButtons);

        var currentPage = 0;
        gridPages.push(pageComponent.createObject(null));

        for (var buttonIndex = 0; buttonIndex < tabletButtons.length; buttonIndex++) {
            var button = tabletButtons[buttonIndex]
            if (button !== null) {
                var grid = gridPages[currentPage];
                if (grid.children[0].children.length === buttonsOnPage) {
                    gridPages.push(pageComponent.createObject(null));
                    currentPage = currentPage + 1;
                    grid = gridPages[currentPage];
                }
                button.row = Math.floor(grid.children[0].children.length / buttonsColumnsOnPage);
                button.column = grid.children[0].children.length % buttonsColumnsOnPage;
                //reparent to actual grid
                button.parent = grid.children[0];
                grid.children[0].children.push(button);
            } else {
                console.warn("Button is null:", buttonIndex);
            }
        }
        for (var pageIndex = 0; pageIndex < gridPages.length; pageIndex++) {
            swipeView.addItem(gridPages[pageIndex]);
        }

        swipeView.setCurrentIndex(0);
    }

    // used to look up a button by its uuid
    function findButtonIndex(uuid) {
        if (!uuid) {
            return -1;
        }
        for (var i in fakeParent.resources) {
            var child = fakeParent.resources[i];
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
        fakeParent.resources.push(button);
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
            fakeParent.resources[index] = null;
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
            currentIndex: -1
            property int previousIndex: -1

            onCurrentIndexChanged: {
                if (swipeView.currentIndex < 0
                        || swipeView.itemAt(swipeView.currentIndex) === null
                        || swipeView.itemAt(swipeView.currentIndex).children[0] === null
                        || swipeView.itemAt(swipeView.currentIndex).children[0].children === null) {
                    return;
                }

                currentGridItems = swipeView.itemAt(swipeView.currentIndex).children[0].children;

                var row = rowIndex < 0 ? 0 : rowIndex;
                var column = previousIndex > swipeView.currentIndex ? buttonsColumnsOnPage - 1 : 0;
                var index = row * buttonsColumnsOnPage + column;
                if (index < 0 || index >= currentGridItems.length) {
                    column = 0;
                    row = 0;
                }
                rowIndex = row;
                columnIndex = column;
                setCurrentItemState("hover state");
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

    function nextItem() {        
        setCurrentItemState("base state");
        var nextColumnIndex = columnIndex + 1;
        var index = rowIndex * buttonsColumnsOnPage + nextColumnIndex;
        if (index >= currentGridItems.length || nextColumnIndex >= buttonsColumnsOnPage) {
            if (swipeView.currentIndex < swipeView.count - 1) {
                swipeView.incrementCurrentIndex();
            } else {
                setCurrentItemState("hover state");
            }
            return;
        }
        columnIndex = nextColumnIndex;
        setCurrentItemState("hover state");
    }

    function previousItem() {
        setCurrentItemState("base state");
        var column = columnIndex - 1;

        if (column < 0) {
            if (swipeView.currentIndex > 0) {
                swipeView.decrementCurrentIndex();
            } else {
                setCurrentItemState("hover state");
            }
            return;
        }
        columnIndex = column;
        setCurrentItemState("hover state");
    }

    function upItem() {
        setCurrentItemState("base state");
        var row = rowIndex - 1;

        if (row < 0 ) {
            row =  buttonsRowsOnPage - 1;
            var index = row * buttonsColumnsOnPage + columnIndex;
            while (index >= currentGridItems.length) {
                row = row - 1;
                index = row * buttonsColumnsOnPage + columnIndex;
            }
        }
        rowIndex = row;
        setCurrentItemState("hover state");
    }

    function downItem() {
        setCurrentItemState("base state");
        rowIndex = rowIndex + 1;
        var index = rowIndex * buttonsColumnsOnPage + columnIndex;
        if (index >= currentGridItems.length) {
            rowIndex = 0;
        }
        setCurrentItemState("hover state");
    }

    function selectItem() {
        var index = rowIndex*buttonsColumnsOnPage + columnIndex;
        if (currentGridItems !== null && index >= 0 && index < currentGridItems.length) {
            currentGridItems[index].clicked();
            if (tabletRoot) {
                tabletRoot.playButtonClickSound();
            }
        }
    }

    Keys.onRightPressed: nextItem();
    Keys.onLeftPressed: previousItem();
    Keys.onDownPressed: downItem();
    Keys.onUpPressed: upItem();
    Keys.onReturnPressed: selectItem();
}
