import "../../styles-uit"
import QtQuick 2.9
import QtGraphicalEffects 1.0

ShadowRectangle {
    width: 44
    height: 28
    AvatarAppStyle {
        id: style
    }

    gradient: Gradient {
        GradientStop { position: 0.0; color: style.colors.blueHighlight }
        GradientStop { position: 1.0; color: style.colors.blueAccent }
    }

    property alias glyphText: glyph.text
    property alias glyphRotation: glyph.rotation
    property alias glyphSize: glyph.size

    radius: 3

    HiFiGlyphs {
        id: glyph
        color: 'white'
        anchors.centerIn: parent
        size: 30
    }
}
