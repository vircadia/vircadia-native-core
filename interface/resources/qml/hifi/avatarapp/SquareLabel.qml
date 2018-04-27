import "../../styles-uit"
import QtQuick 2.9
import QtGraphicalEffects 1.0

ShadowRectangle {
    width: 44
    height: 28
    color: 'white'
    property alias glyphText: glyph.text
    property alias glyphRotation: glyph.rotation
    property alias glyphSize: glyph.size

    radius: 3
    border.color: 'black'
    border.width: 1.5

    HiFiGlyphs {
        id: glyph
        anchors.centerIn: parent
        size: 30
    }
}
