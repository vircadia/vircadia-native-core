import QtQuick 2.5
import QtQuick.Controls 1.4 as Original

Preference {
    id: root
    height: button.height

    Component.onCompleted: button.text = preference.name;

    function save() { }

    Original.Button {
        id: button
        onClicked: preference.trigger()
    }
}
