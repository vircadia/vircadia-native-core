import Hifi 1.0 as Hifi
import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import "controls"
import "styles"

Hifi.VrMenu {
    id: root
    HifiConstants { id: hifi }

    anchors.fill: parent

    objectName: "VrMenu"

    enabled: false
    opacity: 0.0

    property int animationDuration: 200
    property var models: []
    property var columns: []

    z: 10000

    onEnabledChanged: {
        if (enabled && columns.length == 0) {
            pushColumn(rootMenu.items);
        }
        opacity = enabled ? 1.0 : 0.0
        if (enabled) {
            forceActiveFocus()
        }
    }

    // The actual animator
    Behavior on opacity {
        NumberAnimation {
            duration: root.animationDuration
            easing.type: Easing.InOutBounce
        }
    }

    onOpacityChanged: {
        visible = (opacity != 0.0);
    }

    onVisibleChanged: {
        if (!visible) reset();
    }

    property var menuBuilder: Component {
        Border {
            Component.onCompleted: {
                menuDepth = root.models.length - 1
                if (menuDepth == 0) {
                    x = lastMousePosition.x - 20
                    y = lastMousePosition.y - 20
                } else {
                    var lastColumn = root.columns[menuDepth - 1] 
                    x = lastColumn.x + 64;
                    y = lastMousePosition.y - height / 2;
                }
            }
            border.color: hifi.colors.hifiBlue
            color: hifi.colors.window
            property int menuDepth
/*
            MouseArea {
//                Rectangle { anchors.fill: parent; color: "#7f0000FF"; visible: enabled }
                anchors.fill: parent
                onClicked: {
                    while (parent.menuDepth != root.models.length - 1) {
                        root.popColumn()
                    }
                }
            }
*/

            ListView {
                spacing: 6
                property int outerMargin: 8 
                property real minWidth: 0
                anchors.fill: parent
                anchors.margins: outerMargin
                id: listView
                height: root.height

                onCountChanged: {
                    recalculateSize()
                }

                function recalculateSize() {
                    var newHeight = 0
                    var newWidth = minWidth;
                    for (var i = 0; i < children.length; ++i) {
                        var item = children[i];
                        if (!item.visible) {
                            continue
                        }
                        newHeight += item.height
                    }
                    parent.height = newHeight + outerMargin * 2;
                    parent.width = newWidth + outerMargin * 2
                }

                highlight: Rectangle {
                    width: listView.minWidth - 32; 
                    height: 32
                    color: hifi.colors.hifiBlue
                    y: (listView.currentItem) ? listView.currentItem.y : 0;
                    x: 32
                    Behavior on y { 
                          NumberAnimation {
                              duration: 100
                              easing.type: Easing.InOutQuint
                          }
                    }
                }

                model: root.models[menuDepth]
                delegate: Loader {
                    id: loader
                    sourceComponent: root.itemBuilder
                    Binding {
                        target: loader.item
                        property: "root"
                        value: root
                        when: loader.status == Loader.Ready
                    }        
                    Binding {
                        target: loader.item
                        property: "source"
                        value: modelData
                        when: loader.status == Loader.Ready
                    }        
                    Binding {
                        target: loader.item
                        property: "listView"
                        value: listView
                        when: loader.status == Loader.Ready
                    }        
                }
            }
        }
    }

    property var itemBuilder: Component {
        Text {
            id: thisText
            x: 32
            property var source
            property var root
            property var listView
            text: typedText()
            height: implicitHeight
            width: implicitWidth
            color: source.enabled ? hifi.colors.text : hifi.colors.disabledText
            enabled: source.enabled && source.visible
            // FIXME uncommenting this line results in menus that have blank spots
            // rather than having the correct size
            // visible: source.visible

            onListViewChanged: {
                if (listView) {
                    listView.minWidth = Math.max(listView.minWidth, implicitWidth + 64);
                    listView.recalculateSize();
                }
            }

            onVisibleChanged: {
                if (listView) {
                    listView.recalculateSize();
                }
            }

            onImplicitWidthChanged: {
                if (listView) {
                    listView.minWidth = Math.max(listView.minWidth, implicitWidth + 64);
                    listView.recalculateSize();
                }
            }

            FontAwesome {
                visible: source.type == 1 && source.checkable
                x: -32
                text: checkText();  
                color: parent.color
                function checkText() {
                    if (source.type != 1) {
                        return;
                    }
                    // FIXME this works for native QML menus but I don't think it will
                    // for proxied QML menus
                    if (source.exclusiveGroup) {
                        return source.checked ? "\uF05D" : "\uF10C"
                    }
                    return source.checked ? "\uF046" : "\uF096"
                }
            }

            FontAwesome {
                visible: source.type == 2
                x: listView.width - 32 - (hifi.layout.spacing * 2)
                text: "\uF0DA"
                color: parent.color
            }

            function typedText() {
                switch(source.type) {
                case 2:
                    return source.title;
                case 1: 
                    return source.text;
                case 0:
                    return "-----"
                }
            }
            
            MouseArea {
                id: mouseArea
                acceptedButtons: Qt.LeftButton
                anchors.left: parent.left
                anchors.leftMargin: -32
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.top: parent.top
                anchors.topMargin: 0
                width: listView.width
                hoverEnabled: true
                Timer {
                    id: timer
                    interval: 1000
                    onTriggered: parent.select();
                }
                /*  
                 * Uncomment below to have menus auto-popup
                 * 
                 * FIXME if we enabled timer based menu popup, either the timer has 
                 * to be very very short or after auto popup there has to be a small 
                 * amount of time, or a test if the mouse has moved before a click 
                 * will be accepted, otherwise it's too easy to accidently click on
                 * something immediately after the auto-popup appears underneath your 
                 * cursor
                 *
                 */
                //onEntered: {
                //    if (source.type == 2 && enabled) {
                //        timer.start()
                //    }
                //}
                //onExited: {
                //    timer.stop()
                //}
                onClicked: {
                    select();
                }
                function select() {
                    timer.stop();
                    var popped = false;
                    while (columns.length - 1 > listView.parent.menuDepth) {
                        popColumn();
                        popped = true;
                    }

                    if (!popped || source.type != 1) {
                        parent.root.selectItem(parent.source);
                    }
                }
            }
        }
    }

    function lastColumn() {
        return columns[root.columns.length - 1];
    }

    function pushColumn(items) {
        models.push(items)
        if (columns.length) {
            var oldColumn = lastColumn();
            //oldColumn.enabled = false
        }
        var newColumn = menuBuilder.createObject(root); 
        columns.push(newColumn);
        newColumn.forceActiveFocus();
    }

    function popColumn() {
        if (columns.length > 0) {
            var curColumn = columns.pop();
            console.log(curColumn);
            curColumn.visible = false;
            curColumn.destroy();
            models.pop();
        } 
        
        if (columns.length == 0) {
            enabled = false;
            return;
        } 

        curColumn = lastColumn();
        curColumn.enabled = true;
        curColumn.opacity = 1.0;
        curColumn.forceActiveFocus();
    }
    
    function selectItem(source) {
        switch (source.type) {
        case 2:
            pushColumn(source.items)
            break;
        case 1:
            source.trigger()
            enabled = false
            break;
        case 0: 
            break;
        }
    }
    
    function reset() {
        while (columns.length > 0) {
            popColumn();
        }
    }

    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_Escape:
            root.popColumn()
            event.accepted = true;
        }
    }

    MouseArea {
        anchors.fill: parent
        id: mouseArea
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            if (mouse.button == Qt.RightButton) {
                root.popColumn();
            } else {
                root.enabled = false;
            }            
        }
    }
}
