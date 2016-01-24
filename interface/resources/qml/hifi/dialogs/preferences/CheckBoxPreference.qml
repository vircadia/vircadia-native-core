import QtQuick 2.5
import QtQuick.Controls 1.4 as Original
import "."

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

    Original.CheckBox {
        id: checkBox
        anchors.fill: parent
        text: root.label
    }
}
