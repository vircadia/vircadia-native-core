import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebKit 3.0

Item {
    id: root
    implicitHeight: 600
    implicitWidth: 800
    Rectangle {
        anchors.margins: 120
        color: "#7f0000ff"
        anchors.right: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        anchors.bottomMargin: 10
        anchors.topMargin: 10
    }

    Rectangle {
        color: "#7Fff0000"
        anchors.left: parent.horizontalCenter
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.margins: 120
        anchors.right: parent.right
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.bottom: parent.bottom
        anchors.rightMargin: 10
/*
        ScrollView {
            id: scrollView
            anchors.fill: parent
            WebView {
                id: webview
                objectName: "webview"
                anchors.fill: parent
            }
        }
*/
    }
}
