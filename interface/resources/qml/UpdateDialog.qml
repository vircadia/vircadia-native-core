import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls.Styles 1.3
import "controls"
import "styles"

Rectangle {
    id: page
    width: 320; height: 480
    color: "lightgray"

    Text {
        id: helloText
        text: "Hello world!"
        y: 30
        anchors.horizontalCenter: page.horizontalCenter
        font.pointSize: 24; font.bold: true
    }
}