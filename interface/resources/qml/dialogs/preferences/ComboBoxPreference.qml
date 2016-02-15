import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Preference {
    id: root
    property real spacing: 8
    height: labelText.height + dataComboBox.height + spacing

    Component.onCompleted: {
        dataComboBox.currentIndex = dataComboBox.find(preference.value);
    }

    function save() {
        preference.value = dataComboBox.currentText;
        preference.save();
    }

    Text {
        id: labelText
        color: enabled ? "black" : "gray"
        text: root.label
    }

    ComboBox {
        id: dataComboBox
        model: preference.items
        style: ComboBoxStyle { renderType: Text.QtRendering }
        anchors {
            top: labelText.bottom
            left: parent.left
            right: parent.right
            topMargin: root.spacing
            rightMargin: root.spacing
        }
    }
}
