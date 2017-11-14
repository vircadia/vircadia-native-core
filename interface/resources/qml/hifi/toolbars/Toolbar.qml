import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../../windows"
import "."

Window {
    id: window
    frame: ToolFrame {
        horizontalSpacers: horizontal
        verticalSpacers: !horizontal
    }
    property var tabletProxy;
    property var buttonModel: ListModel {}
    hideBackground: true
    resizable: false
    destroyOnCloseButton: false
    destroyOnHidden: false
    closable: false
    shown: true
    width: content.width
    height: content.height
    // Disable this window from being able to call 'desktop.raise() and desktop.showDesktop'
    activator: Item {}
    property bool horizontal: true
    property real buttonSize: 50;

    Settings {
        category: "toolbar/" + window.objectName
        property alias x: window.x
        property alias y: window.y
    }
    
    Component {
        id: buttonComponent
        ToolbarButton {
            id: toolbarButton 
            property var proxy: modelData;
            onClicked: proxy.clicked()
            Component.onCompleted: updateProperties()

            Connections {
                target: proxy;
                onPropertiesChanged: updateProperties();
            }
            
            function updateProperties() {
                Object.keys(proxy.properties).forEach(function (key) {
                    if (toolbarButton[key] !== proxy.properties[key]) {
                        toolbarButton[key] = proxy.properties[key];
                    }
                });
            }
        }
    }

    Item {
        id: content
        implicitHeight: horizontal ? row.height : column.height
        implicitWidth: horizontal ? row.width : column.width

        Row {
            id: row
            visible: window.horizontal
            spacing: 6
            Repeater {
                model: buttonModel
                delegate: buttonComponent
            }
        }
        
        Column {
            id: column
            visible: !window.horizontal 
            spacing: 6
            Repeater {
                model: buttonModel
                delegate: buttonComponent
            }
        }
    }
}
