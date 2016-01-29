import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2

Button {
    property var dialog;
    property int button: StandardButton.NoButton;

    focus: dialog.defaultButton === button
    onClicked: dialog.click(button)
    visible: dialog.buttons & button
}
