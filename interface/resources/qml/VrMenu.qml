import Hifi 1.0 as Hifi
import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import "controls"
import "styles"

Hifi.VrMenu {
    id: root
    anchors.fill: parent
    objectName: "VrMenu"
    enabled: false
    opacity: 0.0
    property int animationDuration: 200
    HifiPalette { id: hifiPalette }
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


    property var models: []
    property var columns: []
    
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
            SystemPalette { id: sysPalette; colorGroup: SystemPalette.Active }
            border.color: hifiPalette.hifiBlue
            color: sysPalette.window
            property int menuDepth

            ListView {
                spacing: 6
                property int outerMargin: 8 
                property real minWidth: 0
                anchors.fill: parent
                anchors.margins: outerMargin
                id: listView
                height: root.height
                currentIndex: -1

                onCountChanged: {
                    recalculateSize()
                }                 
                
                function recalculateSize() {
                    var newHeight = 0
                    var newWidth = minWidth;
                    for (var i = 0; i < children.length; ++i) {
                        var item = children[i];
                        newHeight += item.height
                    }
                    parent.height = newHeight + outerMargin * 2;
                    parent.width = newWidth + outerMargin * 2
                }
            
                highlight: Rectangle {
                    width: listView.minWidth - 32; 
                    height: 32
                    color: sysPalette.highlight
                    y: (listView.currentItem) ? listView.currentItem.y : 0;
                    x: 32
                    Behavior on y { 
                          NumberAnimation {
                              duration: 100
                              easing.type: Easing.InOutQuint
                          }
                    }
                }
           

                property int columnIndex: root.models.length - 1 
                model: root.models[columnIndex]
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
                        property: "listViewIndex"
                        value: index
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
            SystemPalette { id: sp; colorGroup: SystemPalette.Active }
            id: thisText
            x: 32
            property var source
            property var root
            property var listViewIndex
            property var listView
            text: typedText()
            height: implicitHeight
            width: implicitWidth
            color: source.enabled ? "black" : "gray"
                
            onImplicitWidthChanged: {
                if (listView) {
                    listView.minWidth = Math.max(listView.minWidth, implicitWidth + 64);
                    listView.recalculateSize();
                }
            }

            FontAwesome {
                visible: source.type == 1 && source.checkable
                x: -32
                text: (source.type == 1 && source.checked) ? "\uF05D" : "\uF10C"
            }
            
            FontAwesome {
                visible: source.type == 2
                x: listView.width - 64
                text: "\uF0DA"
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
                onEntered: {
                    if (source.type == 2 && enabled) {
                        timer.start()
                    }
                }
                onExited: {
                    timer.stop()
                }
                onClicked: {
                    select();
                }
                function select() {
                    timer.stop();
                    listView.currentIndex = listViewIndex
                    parent.root.selectItem(parent.source);
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
            oldColumn.enabled = false;
            oldColumn.opacity = 0.5;
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
