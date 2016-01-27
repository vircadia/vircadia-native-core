import QtQuick 2.5
import QtQuick.Controls 1.4

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

    signal valueChanged();

    SpinBox {
        id: xspinner
        width: root.spinboxWidth
        anchors { left: parent.left }
        value: root.vector.x

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
    }

    SpinBox {
        id: yspinner
        width: root.spinboxWidth
        anchors { horizontalCenter: parent.horizontalCenter }
        value: root.vector.y

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
    }

    SpinBox {
        id: zspinner
        width: root.spinboxWidth
        anchors { right: parent.right; }
        value: root.vector.z

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
    }
}

