import QtQuick 2.5
import QtQuick.Controls 2.2
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit

TextField {
    id: textField

    property bool error: false;
    text: 'ThisIsDisplayName'

    states: [
        State {
            name: "hovered"
            when: textField.hovered && !textField.focus && !textField.error;
            PropertyChanges { target: background; color: '#afafaf' }
        },
        State {
            name: "focused"
            when: textField.focus && !textField.error
            PropertyChanges { target: background; color: '#f2f2f2' }
            PropertyChanges { target: background; border.color: '#00b4ef' }
        },
        State {
            name: "error"
            when: textField.error
            PropertyChanges { target: background; color: '#f2f2f2' }
            PropertyChanges { target: background; border.color: '#e84e62' }
        }
    ]

    background: Rectangle {
        id: background
        implicitWidth: 200
        implicitHeight: 40
        color: '#d4d4d4'
        border.color: '#afafaf'
        border.width: 1
        radius: 2
    }

    HiFiGlyphs {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        size: 36
        text: "\ue00d"
    }
}
