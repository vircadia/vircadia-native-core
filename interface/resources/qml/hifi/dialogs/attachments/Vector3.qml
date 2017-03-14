import QtQuick 2.5
import QtQuick.Controls 1.4

import "../../../styles-uit"
import "../../../controls-uit" as HifiControls
import "../../../windows"

Item {
    id: root
    implicitHeight: xspinner.height
    readonly property real spacing: 8
    property real spinboxWidth: (width / 3) - spacing
    property var vector;
    property real decimals: 0
    property real stepSize: 1
    property real maximumValue: 99
    property real minimumValue: 0
    property bool controlFocus: false;  // True if one of the ordinate controls has focus.
    property var controlFocusControl: undefined

    signal valueChanged();

    function setControlFocus() {
        if (controlFocusControl) {
            controlFocusControl.focus = true;
            // The controlFocus value is updated via onFocusChanged.
        }
    }

    function setFocus(control, focus) {
        if (focus) {
            controlFocusControl = control;
            setControlFocusTrue.start();  // After any subsequent false from previous control.
        } else {
            controlFocus = false;
        }
    }

    Timer {
        id: setControlFocusTrue
        interval: 50
        repeat: false
        running: false
        onTriggered: {
            controlFocus = true;
        }
    }

    HifiConstants { id: hifi }

    HifiControls.SpinBox {
        id: xspinner
        width: root.spinboxWidth
        anchors { left: parent.left }
        value: root.vector.x
        labelInside: "X:"
        colorScheme: hifi.colorSchemes.dark
        colorLabelInside: hifi.colors.redHighlight
        decimals: root.decimals
        stepSize: root.stepSize
        maximumValue: root.maximumValue
        minimumValue: root.minimumValue
        onValueChanged: {
            if (value !== vector.x) {
                vector.x = value
                root.valueChanged();
            }
        }
        onFocusChanged: setFocus(this, focus);
    }

    HifiControls.SpinBox {
        id: yspinner
        width: root.spinboxWidth
        anchors { horizontalCenter: parent.horizontalCenter }
        value: root.vector.y
        labelInside: "Y:"
        colorLabelInside: hifi.colors.greenHighlight
        colorScheme: hifi.colorSchemes.dark
        decimals: root.decimals
        stepSize: root.stepSize
        maximumValue: root.maximumValue
        minimumValue: root.minimumValue
        onValueChanged: {
            if (value !== vector.y) {
                vector.y = value
                root.valueChanged();
            }
        }
        onFocusChanged: setFocus(this, focus);
    }

    HifiControls.SpinBox {
        id: zspinner
        width: root.spinboxWidth
        anchors { right: parent.right; }
        value: root.vector.z
        labelInside: "Z:"
        colorLabelInside: hifi.colors.primaryHighlight
        colorScheme: hifi.colorSchemes.dark
        decimals: root.decimals
        stepSize: root.stepSize
        maximumValue: root.maximumValue
        minimumValue: root.minimumValue
        onValueChanged: {
            if (value !== vector.z) {
                vector.z = value
                root.valueChanged();
            }
        }
        onFocusChanged: setFocus(this, focus);
    }
}
