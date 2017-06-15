import QtQuick 2.2
import QtQuick.Window 2.0
import QtQuick.Layouts 1.1

import "ColorUtils.js" as ColorUtils

Item {
    id: root
    width: parent.width
    height: parent.height * 0.666
    focus: true

    // Color value in RGBA with floating point values between 0.0 and 1.0.
	
	
    property vector4d colorHSVA: Qt.vector4d(1, 1, 1, 1)
    QtObject {
        id: m
        // Color value in HSVA with floating point values between 0.0 and 1.0.
        property vector4d colorRGBA: ColorUtils.hsva2rgba(root.colorHSVA)
    }

    signal accepted

    onAccepted: {
		var rgba = ColorUtils.hsva2rgba(root.colorHSVA)
		parent.sendToScript(["color", rgba.x*255 , rgba.y*255, rgba.z*255]);
    }

    RowLayout {
        spacing: 20
        anchors.fill: parent

        Wheel {
            id: wheel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.minimumHeight: 200

            hue: colorHSVA.x
            saturation: colorHSVA.y
            onUpdateHS: {
                colorHSVA = Qt.vector4d(hueSignal,saturationSignal, colorHSVA.z, colorHSVA.w)
            }
            onAccepted: {
                root.accepted()
            }
        }

        // brightness picker slider
        Item {
            Layout.fillHeight: true
            Layout.minimumWidth: 20
            Layout.minimumHeight: 200

            //Brightness background
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop {
                        id: brightnessBeginColor
                        position: 0.0
                        color: {
                            var rgba = ColorUtils.hsva2rgba(
                                        Qt.vector4d(colorHSVA.x,
                                                    colorHSVA.y, 1, 1))
                            return Qt.rgba(rgba.x, rgba.y, rgba.z, rgba.w)
                        }
                    }
                    GradientStop {
                        position: 1.0
                        color: "#000000"
                    }
                }
            }

            VerticalSlider {
                id: brigthnessSlider
                anchors.fill: parent
                value: colorHSVA.z
                onValueChanged: {
                    colorHSVA = Qt.vector4d(colorHSVA.x, colorHSVA.y, value, colorHSVA.w)
                }
                onAccepted: {
                    root.accepted()
                }
            }
        }


        // text inputs
        ColumnLayout {
            Layout.fillHeight: true
            Layout.minimumWidth: 150
            Layout.minimumHeight: 200
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            // current color display
            Rectangle {
                Layout.minimumWidth: 150
                Layout.minimumHeight: 50
                CheckerBoard {
                    cellSide: 5
                }
                Rectangle {
                    id: colorDisplay
                    width: parent.width
                    height: parent.height
                    border.width: 1
                    border.color: "black"
                    color: Qt.rgba(m.colorRGBA.x, m.colorRGBA.y, m.colorRGBA.z)
                    opacity: m.colorRGBA.w
                }
            }


            // current color value
            Item {
                Layout.minimumWidth: 120
                Layout.minimumHeight: 25

                Text {
                    id: captionBox
                    text: "#"
                    width: 18
                    height: parent.height
                    color: "#AAAAAA"
                    font.pixelSize: 16
                    font.bold: true
                }
                PanelBorder {
                    height: parent.height
                    anchors.left : captionBox.right
                    width: parent.width - captionBox.width
                    TextInput {
                        id: currentColor
                        color: "#AAAAAA"
                        selectionColor: "#FF7777AA"
                        font.pixelSize: 16
                        font.capitalization: "AllUppercase"
                        maximumLength: 9
                        focus: true
                        text: ColorUtils.hexaFromRGBA(m.colorRGBA.x, m.colorRGBA.y,
                                                      m.colorRGBA.z, m.colorRGBA.w)
                        font.family: "Droid Sans"
                        selectByMouse: true
                        validator: RegExpValidator {
                            regExp: /^([A-Fa-f0-9]{6})$/
                        }
                        onEditingFinished: {
                            var colorTmp = Qt.vector4d( parseInt(text.substr(0, 2), 16) / 255,
                                                    parseInt(text.substr(2, 2), 16) / 255,
                                                    parseInt(text.substr(4, 2), 16) / 255,
                                                    colorHSVA.w) ;
                            colorHSVA = ColorUtils.rgba2hsva(colorTmp)
                        }
                    }
                }
            }
            // H, S, B color value boxes
            Column {
                Layout.minimumWidth: 80
                Layout.minimumHeight: 25
                NumberBox {
                    id: hue
                    caption: "H"
                    // TODO: put in NumberBox
                    value: Math.round(colorHSVA.x * 100000) / 100000 // 5 Decimals
                    decimals: 2
                    max: 1
                    min: 0
                    onAccepted: {
                        colorHSVA =  Qt.vector4d(boxValue, colorHSVA.y, colorHSVA.z, colorHSVA.w)
                        root.accepted()
                    }
                }
                NumberBox {
                    id: sat
                    caption: "S"
                    value: Math.round(colorHSVA.y * 100) / 100 // 2 Decimals
                    decimals: 2
                    max: 1
                    min: 0
                    onAccepted: {
                        colorHSVA = Qt.vector4d(colorHSVA.x, boxValue, colorHSVA.z, colorHSVA.w)
                        root.accepted()
                    }
                }
                NumberBox {
                    id: brightness
                    caption: "B"
                    value: Math.round(colorHSVA.z * 100) / 100 // 2 Decimals
                    decimals: 2
                    max: 1
                    min: 0
                    onAccepted: {
                        colorHSVA = Qt.vector4d(colorHSVA.x, colorHSVA.y, boxValue, colorHSVA.w)
                        root.accepted()
                    }
                }
                NumberBox {
                    id: hsbAlpha
                    caption: "A"
                    value: Math.round(colorHSVA.w * 100) / 100 // 2 Decimals
                    decimals: 2
                    max: 1
                    min: 0
                    onAccepted: {
                        colorHSVA.w = boxValue
                        root.accepted()
                    }
                }
            }

            // R, G, B color values boxes
            Column {
                Layout.minimumWidth: 80
                Layout.minimumHeight: 25
                NumberBox {
                    id: red
                    caption: "R"
                    value: Math.round(m.colorRGBA.x * 255)
                    min: 0
                    max: 255
                    decimals: 0
                    onAccepted: {
                        var colorTmp = Qt.vector4d( boxValue / 255,
                                                    m.colorRGBA.y,
                                                    m.colorRGBA.z,
                                                    colorHSVA.w) ;
                        colorHSVA = ColorUtils.rgba2hsva(colorTmp)
                        root.accepted()
                    }
                }
                NumberBox {
                    id: green
                    caption: "G"
                    value: Math.round(m.colorRGBA.y * 255)
                    min: 0
                    max: 255
                    decimals: 0
                    onAccepted: {
                        var colorTmp = Qt.vector4d( m.colorRGBA.x,
                                                    boxValue / 255,
                                                    m.colorRGBA.z,
                                                    colorHSVA.w) ;
                        colorHSVA = ColorUtils.rgba2hsva(colorTmp)
                        root.accepted()
                    }
                }
                NumberBox {
                    id: blue
                    caption: "B"
                    value: Math.round(m.colorRGBA.z * 255)
                    min: 0
                    max: 255
                    decimals: 0
                    onAccepted: {
                        var colorTmp = Qt.vector4d( m.colorRGBA.x,
                                                    m.colorRGBA.y,
                                                    boxValue / 255,
                                                    colorHSVA.w) ;
                        colorHSVA = ColorUtils.rgba2hsva(colorTmp)
                        root.accepted()
                    }
                }
                NumberBox {
                    id: rgbAlpha
                    caption: "A"
                    value: Math.round(m.colorRGBA.w * 255)
                    min: 0
                    max: 255
                    decimals: 0
                    onAccepted: {
                        root.colorHSVA.w = boxValue / 255
                        root.accepted()
                    }
                }
            }
        }
    }
}
