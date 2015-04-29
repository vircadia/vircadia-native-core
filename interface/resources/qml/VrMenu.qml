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
            HifiConstants { id: hifi }
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
            implicitHeight: listView.implicitHeight + 16
            implicitWidth: listView.implicitWidth + 16

            Column {
                id: listView
                property real minWidth: 0
                anchors {
                    top: parent.top
                    topMargin: 8
                    left: parent.left
                    leftMargin: 8
                    right: parent.right
                    rightMargin: 8
                }

                Repeater {
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
                            property: "border"
                            value: listView.parent
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
    }

    property var itemBuilder: Component {
        Item {
            property var source
            property var root
            property var listView
            property var border



            implicitHeight: row.implicitHeight + 4
            implicitWidth: row.implicitWidth + label.height 
            // FIXME uncommenting this line results in menus that have blank spots
            // rather than having the correct size
            // visible: source.visible
            Row {
                id: row
                spacing: 2
                anchors {
                    top: parent.top
                    topMargin: 2
                }
                Spacer { size: 4 }
                FontAwesome {
                    id: check
                    verticalAlignment: Text.AlignVCenter
                    size: row.height
                    text: checkText()
                    color: label.color
                    function checkText() {
                        if (!source || source.type != 1 || !source.checkable) {
                            return "";
                        }

                        // FIXME this works for native QML menus but I don't think it will
                        // for proxied QML menus
                        if (source.exclusiveGroup) {
                            return source.checked ? "\uF05D" : "\uF10C"
                        }
                        return source.checked ? "\uF046" : "\uF096"
                    }
                }
                Text {
                  id: label
//                  anchors.top: parent.top
//                  anchors.bottom: parent.bottom
                  text: typedText()
                  verticalAlignment: Text.AlignVCenter
                  color: source.enabled ? hifi.colors.text : hifi.colors.disabledText
                  enabled: source.enabled && source.visible
                  function typedText() {
                      if (source) {
                          switch(source.type) {
                          case 2:
                              return source.title;
                          case 1: 
                              return source.text;
                          case 0:
                              return "-----"
                          }
                      }
                      return ""
                  }
              }
           } // row

           FontAwesome {
               anchors {
                   top: row.top
               }
               id: tag
               size: label.height
               width: implicitWidth
               visible: source.type == 2
               x: listView.width - width - 4
               text: "\uF0DA"
               color: label.color
           }
            
           MouseArea {
               anchors.top: parent.top
               anchors.bottom: parent.bottom
               anchors.right: tag.right
               anchors.left: parent.left
               anchors.rightMargin: -4
               acceptedButtons: Qt.LeftButton
               hoverEnabled: true
               Rectangle {
                   id: highlight
                   visible: false
                   anchors.fill: parent 
                   color: "#7f0e7077"
               }
               Timer {
                   id: timer
                   interval: 1000
                   onTriggered: parent.select();
               }
               onEntered: {
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
                   //if (source.type == 2 && enabled) {
                   //    timer.start()
                   //}
                   highlight.visible = source.enabled
               }
               onExited: {
                   timer.stop()
                   highlight.visible = false
               }
               onClicked: {
                   select();
               }
               function select() {
                   //timer.stop();
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
