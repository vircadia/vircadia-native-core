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
            varying highp vec2 qt_TexCoord0;
            uniform lowp sampler2D source;
            uniform lowp sampler2D mask;
            void main() {

                highp vec4 maskColor = texture2D(mask, vec2(qt_TexCoord0.x, qt_TexCoord0.y));
                highp vec4 sourceColor = texture2D(source, vec2(qt_TexCoord0.x, qt_TexCoord0.y));

                if (maskColor.a > 0.0)
                    gl_FragColor = sourceColor;
                else
                    gl_FragColor = maskColor;
            }
"
        }
    }
}