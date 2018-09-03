import QtQuick 2.3
import QtQuick.Controls 1.2

Rectangle {
    color: "teal"
    height: 512
    width: 192
    SystemPalette { id: sp; colorGroup: SystemPalette.Active }
    SystemPalette { id: spi; colorGroup: SystemPalette.Inactive }
    SystemPalette { id: spd; colorGroup: SystemPalette.Disabled }

    Column {
        anchors.margins: 8
        anchors.fill: parent
        spacing: 8
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "base" }
            Rectangle { height: parent.height; width: 16; color: sp.base }
            Rectangle { height: parent.height; width: 16; color: spi.base }
            Rectangle { height: parent.height; width: 16; color: spd.base }
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "alternateBase" }
            Rectangle { height: parent.height; width: 16; color: sp.alternateBase }
            Rectangle { height: parent.height; width: 16; color: spi.alternateBase }
            Rectangle { height: parent.height; width: 16; color: spd.alternateBase }
        }
        Item {
            height: 16
            width:parent.width
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "dark" }
            Rectangle { height: parent.height; width: 16; color: sp.dark }
            Rectangle { height: parent.height; width: 16; color: spi.dark }
            Rectangle { height: parent.height; width: 16; color: spd.dark }
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "mid" }
            Rectangle { height: parent.height; width: 16; color: sp.mid }
            Rectangle { height: parent.height; width: 16; color: spi.mid }
            Rectangle { height: parent.height; width: 16; color: spd.mid }
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "mid light" }
            Rectangle { height: parent.height; width: 16; color: sp.midlight }
            Rectangle { height: parent.height; width: 16; color: spi.midlight }
            Rectangle { height: parent.height; width: 16; color: spd.midlight }
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "light" }
            Rectangle { height: parent.height; width: 16; color: sp.light}
            Rectangle { height: parent.height; width: 16; color: spi.light}
            Rectangle { height: parent.height; width: 16; color: spd.light}
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "shadow" }
            Rectangle { height: parent.height; width: 16; color: sp.shadow}
            Rectangle { height: parent.height; width: 16; color: spi.shadow}
            Rectangle { height: parent.height; width: 16; color: spd.shadow}
        }
        Item {
            height: 16
            width:parent.width
        }

        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "text" }
            Rectangle { height: parent.height; width: 16; color: sp.text }
            Rectangle { height: parent.height; width: 16; color: spi.text }
            Rectangle { height: parent.height; width: 16; color: spd.text }
        }
        Item {
            height: 16
            width:parent.width
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "window" }
            Rectangle { height: parent.height; width: 16; color: sp.window }
            Rectangle { height: parent.height; width: 16; color: spi.window }
            Rectangle { height: parent.height; width: 16; color: spd.window }
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "window text" }
            Rectangle { height: parent.height; width: 16; color: sp.windowText }
            Rectangle { height: parent.height; width: 16; color: spi.windowText }
            Rectangle { height: parent.height; width: 16; color: spd.windowText }
        }
        Item {
            height: 16
            width:parent.width
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "button" }
            Rectangle { height: parent.height; width: 16; color: sp.button }
            Rectangle { height: parent.height; width: 16; color: spi.button }
            Rectangle { height: parent.height; width: 16; color: spd.button }
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "buttonText" }
            Rectangle { height: parent.height; width: 16; color: sp.buttonText }
            Rectangle { height: parent.height; width: 16; color: spi.buttonText }
            Rectangle { height: parent.height; width: 16; color: spd.buttonText }
        }
        Item {
            height: 16
            width:parent.width
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "highlight" }
            Rectangle { height: parent.height; width: 16; color: sp.highlight }
            Rectangle { height: parent.height; width: 16; color: spi.highlight }
            Rectangle { height: parent.height; width: 16; color: spd.highlight }
        }
        Row {
            width: parent.width
            height: 16
            Text { height: parent.height;  width: 128; text: "highlighted text" }
            Rectangle { height: parent.height; width: 16; color: sp.highlightedText}
            Rectangle { height: parent.height; width: 16; color: spi.highlightedText}
            Rectangle { height: parent.height; width: 16; color: spd.highlightedText}
        }
    }
}
