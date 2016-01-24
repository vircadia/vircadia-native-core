import QtQuick 2.5
import QtQuick.Controls 1.4

Preference {
    id: root
    property alias slider: slider
    height: slider.height

    Component.onCompleted: {
        slider.value = preference.value;
    }

    function save() {
        preference.value = slider.value;
        preference.save();
    }

    Text {
        text: root.label
        color: enabled ? "black" : "gray"
        anchors.verticalCenter: slider.verticalCenter
    }

    Slider {
        id: slider
        value: preference.value
        width: 130
        anchors { right: parent.right }
    }
}
