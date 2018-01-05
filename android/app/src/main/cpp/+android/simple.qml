import QtQuick 2.7
import QtWebView 1.1

Rectangle {
    id: window
    anchors.fill: parent
    color: "red"
    ColorAnimation on color { from: "blue"; to: "yellow"; duration: 1000; loops: Animation.Infinite }

    Text {
        text: "Hello"
        anchors.top: parent.top
    }
    WebView {
        anchors.fill: parent
        anchors.margins: 10
        url: "http://doc.qt.io/qt-5/qml-qtwebview-webview.html"
    }
}
