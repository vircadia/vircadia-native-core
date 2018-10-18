import QtQuick 2.0

Item {
    property alias source: sourceImage.sourceItem
    property alias maskSource: sourceMask.sourceItem

    anchors.fill: parent
    ShaderEffectSource {
        id: sourceMask
        smooth: true
        hideSource: true
    }
    ShaderEffectSource {
        id: sourceImage
        hideSource: true
    }

    ShaderEffect {
        id: maskEffect
        anchors.fill: parent

        property variant source: sourceImage
        property variant mask: sourceMask

        fragmentShader: {
"
#version 410
in vec2 qt_TexCoord0;
out vec4 color;
uniform sampler2D source;
uniform sampler2D mask;
void main()
{
    vec4 maskColor = texture(mask, vec2(qt_TexCoord0.x, qt_TexCoord0.y));
    vec4 sourceColor = texture(source, vec2(qt_TexCoord0.x, qt_TexCoord0.y));
    if (maskColor.a > 0.0)
        color = sourceColor;
    else
        color = maskColor;
}
"
        }
    }
}