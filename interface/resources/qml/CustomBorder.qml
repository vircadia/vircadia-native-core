import QtQuick 2.3


Rectangle {
    SystemPalette { id: myPalette; colorGroup: SystemPalette.Active }
    property int margin: 5
    color: myPalette.window
    border.color: myPalette.dark
    border.width: 5
    radius: border.width * 2
}

