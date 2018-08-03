import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import QtQuick 2.9
import QtGraphicalEffects 1.0

Item {
    id: root
    width: 44
    height: 28
    signal clicked();

    HifiControlsUit.Button {
        id: button

        HifiConstants {
            id: hifi
        }

        anchors.fill: parent
        color: hifi.buttons.blue;
        colorScheme: hifi.colorSchemes.light;
        radius: 3
        onClicked: root.clicked();
    }

    DropShadow {
        id: shadow
        anchors.fill: button
        radius: 6
        horizontalOffset: 0
        verticalOffset: 3
        color: Qt.rgba(0, 0, 0, 0.25)
        source: button
    }

    property alias glyphText: glyph.text
    property alias glyphRotation: glyph.rotation
    property alias glyphSize: glyph.size

    HiFiGlyphs {
        id: glyph
        color: 'white'
        anchors.centerIn: parent
        size: 30
    }
}
