import QtQuick 2.5
import QtQuick.Controls 1.4

Preference {
    id: root
    height: checkBox.implicitHeight

    Component.onCompleted: {
        checkBox.checked = preference.value;
        preference.value = Qt.binding(function(){ return checkBox.checked; });
    }

    function save() {
        preference.value = checkBox.checked;
        preference.save();
    }

    CheckBox {
        id: checkBox
        anchors.fill: parent
        text: root.label
    }
}
