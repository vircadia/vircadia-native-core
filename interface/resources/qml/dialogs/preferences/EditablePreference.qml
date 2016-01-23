import QtQuick 2.5
import QtQuick.Controls 1.4

Preference {
    id: root
    property real spacing: 8
    height: labelText.height + dataTextField.height + spacing

    Component.onCompleted: {
        dataTextField.text = preference.value;
    }

    function save() {
        preference.value = dataTextField.text;
        preference.save();
    }

    Text {
        id: labelText
        color: enabled ? "black" : "gray"
        text: root.label
    }

    TextField {
        id: dataTextField
        placeholderText: preference.placeholderText
        anchors {
            top: labelText.bottom
            left: parent.left
            right: parent.right
            topMargin: root.spacing
            rightMargin: root.spacing
        }
    }
}
