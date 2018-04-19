import Hifi 1.0 as Hifi
import QtQuick 2.5
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls

Row {
    width: parent.width
    height: xspinner.controlHeight

    property int spinboxSpace: 10
    property int spinboxWidth: (parent.width - 2 * spinboxSpace) / 3
    property color backgroundColor: "darkgray"

    spacing: spinboxSpace

    property real xvalue: xspinner.value
    property real yvalue: yspinner.value
    property real zvalue: zspinner.value

    HifiControlsUit.SpinBox {
        id: xspinner
        width: parent.spinboxWidth
        labelInside: "X:"
        backgroundColor: parent.backgroundColor
        colorLabelInside: hifi.colors.redHighlight
        colorScheme: hifi.colorSchemes.light
    }

    HifiControlsUit.SpinBox {
        id: yspinner
        width: parent.spinboxWidth
        labelInside: "Y:"
        backgroundColor: parent.backgroundColor
        colorLabelInside: hifi.colors.greenHighlight
        colorScheme: hifi.colorSchemes.light
    }

    HifiControlsUit.SpinBox {
        id: zspinner
        width: parent.spinboxWidth
        labelInside: "Z:"
        backgroundColor: parent.backgroundColor
        colorLabelInside: hifi.colors.primaryHighlight
        colorScheme: hifi.colorSchemes.light
    }
}
