import QtQuick 2.7
import QtQuick.Controls 2.2

StackView {
    id: stackView
    anchors.fill: parent
    anchors.leftMargin: 10
    anchors.rightMargin: 10
    anchors.topMargin: 40

    signal sendToScript(var message);

    NewModelDialog {
        id: dialog
        anchors.fill: parent
        Component.onCompleted:{
            dialog.sendToScript.connect(stackView.sendToScript);
        }
    }
}
