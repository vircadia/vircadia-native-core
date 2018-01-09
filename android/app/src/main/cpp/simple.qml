import QtQuick 2.0

Rectangle {
    id: window
    width: 320
    height: 480
    focus: true
    color: "red"
    ColorAnimation on color { from: "red"; to: "yellow"; duration: 1000; loops: Animation.Infinite }
}
