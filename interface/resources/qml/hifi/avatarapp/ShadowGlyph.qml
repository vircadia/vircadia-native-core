import stylesUit 1.0
import QtQuick 2.9
import QtGraphicalEffects 1.0

Item {
    property alias text: glyph.text
    property alias font: glyph.font
    property alias color: glyph.color
    property alias horizontalAlignment: glyph.horizontalAlignment
    property alias dropShadowRadius: shadow.radius
    property alias dropShadowHorizontalOffset: shadow.horizontalOffset
    property alias dropShadowVerticalOffset: shadow.verticalOffset

    HiFiGlyphs {
        id: glyph
        width: parent.width
        height: parent.height
    }

    DropShadow {
        id: shadow
        anchors.fill: glyph
        radius: 4
        horizontalOffset: 0
        verticalOffset: 4
        color: Qt.rgba(0, 0, 0, 0.25)
        source: glyph
    }
}
