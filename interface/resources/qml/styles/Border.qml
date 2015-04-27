import QtQuick 2.3

Rectangle {
    HifiConstants { id: hifi }
    implicitHeight: 64
    implicitWidth: 64
    color: hifi.colors.window
    border.color: hifi.colors.hifiBlue
    border.width: hifi.styles.borderWidth
    radius: hifi.styles.borderRadius
}

