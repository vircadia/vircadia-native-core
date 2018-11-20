import QtQuick 2.9
import QtQuick.Controls 2.2
import QtWebView 1.1

WebView {
    url: "https://old.reddit.com"
    width: 640; height: 480

    Rectangle {
        id: blue
        color: "blue"
        anchors { margins: 48; top: parent.top; bottom: parent.bottom; left: parent.left;  }
        width: parent.width / 3
        ColorAnimation on color {
            from: "yellow";
            to: "red";
            loops:  Animation.Infinite;
            duration: 1000;
        }
    }
}
