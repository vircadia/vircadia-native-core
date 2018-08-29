import Hifi 1.0 as Hifi
import QtQuick 2.5
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls

Row {
    id: root
    width: parent.width
    height: xspinner.controlHeight

    property int spinboxSpace: 10
    property int spinboxWidth: (parent.width - 2 * spinboxSpace) / 3
    property color backgroundColor: "darkgray"

    property int decimals: 4
    property real realFrom: 0
    property real realTo: 100
    property real realStepSize: 0.0001

    spacing: spinboxSpace

    property bool enabled: false;
    property alias xvalue: xspinner.realValue
    property alias yvalue: yspinner.realValue
    property alias zvalue: zspinner.realValue

    HifiControlsUit.SpinBox {
        id: xspinner
        width: parent.spinboxWidth
        labelInside: "X:"
        backgroundColor: parent.backgroundColor
        colorLabelInside: hifi.colors.redHighlight
        colorScheme: hifi.colorSchemes.light
        decimals: root.decimals;
        realFrom: root.realFrom
        realTo: root.realTo
        realStepSize: root.realStepSize
        enabled: root.enabled
    }

    HifiControlsUit.SpinBox {
        id: yspinner
        width: parent.spinboxWidth
        labelInside: "Y:"
        backgroundColor: parent.backgroundColor
        colorLabelInside: hifi.colors.greenHighlight
        colorScheme: hifi.colorSchemes.light
        decimals: root.decimals;
        realFrom: root.realFrom
        realTo: root.realTo
        realStepSize: root.realStepSize
        enabled: root.enabled
    }

    HifiControlsUit.SpinBox {
        id: zspinner
        width: parent.spinboxWidth
        labelInside: "Z:"
        backgroundColor: parent.backgroundColor
        colorLabelInside: hifi.colors.primaryHighlight
        colorScheme: hifi.colorSchemes.light
        decimals: root.decimals;
        realFrom: root.realFrom
        realTo: root.realTo
        realStepSize: root.realStepSize
        enabled: root.enabled
    }
}
