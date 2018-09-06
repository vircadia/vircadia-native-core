import QtQuick 2.9

Row {
    id: root
    property string yesText;
    property string noText;
    property var onYesClicked;
    property var onNoClicked;

    property alias yesButton: yesButton
    property alias noButton: noButton

    height: childrenRect.height
    layoutDirection: Qt.RightToLeft

    spacing: 30

    BlueButton {
        id: yesButton;
        text: yesText;
        onClicked: {
            console.debug('bluebutton.clicked', onYesClicked);

            if(onYesClicked) {
                onYesClicked();
            }
        }
    }

    WhiteButton {
        id: noButton
        text: noText;
        onClicked: {
            console.debug('whitebutton.clicked', onNoClicked);

            if (onNoClicked) {
                onNoClicked();
            }
        }
    }
}
