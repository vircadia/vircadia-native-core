import QtQuick 2.5
import QtQuick.Controls 1.4

import "."

Preference {
    id: root
    property alias buttonText: button.text
    property alias text: dataTextField.text
    property alias placeholderText: dataTextField.placeholderText
    property real spacing: 8
    height: labelText.height + Math.max(dataTextField.height, button.height) + spacing

    Component.onCompleted: {
        dataTextField.text = preference.value;
    }

    function save() {
        preference.value = dataTextField.text;
        preference.save();
    }

    Text {
        id: labelText
        text:  root.label
    }

    TextField {
        id: dataTextField
        placeholderText: root.placeholderText
        text: preference.value
        anchors {
            top: labelText.bottom
            left: parent.left
            right: button.left
            topMargin: root.spacing
            rightMargin: root.spacing
        }
    }

    Button {
        id: button
        anchors {
            right: parent.right;
            verticalCenter: dataTextField.verticalCenter
        }
        text: "Browse"
    }
}
