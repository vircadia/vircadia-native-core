import QtQuick 2.5
import QtQuick.Controls 1.4

Preference {
    id: root
    property alias spinner: spinner
    height: spinner.height

    Component.onCompleted: {
        spinner.value = preference.value;
    }

    function save() {
        preference.value = spinner.value;
        preference.save();
    }

    Text {
        text: root.label
        color: root.enabled ? "black" : "gray"
        anchors.verticalCenter: spinner.verticalCenter
    }

    SpinBox {
        id: spinner
        decimals: preference.decimals
        minimumValue: preference.min
        maximumValue: preference.max
        width: 100
        anchors { right: parent.right }
    }
}
