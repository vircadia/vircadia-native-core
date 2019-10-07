import QtQuick 2.3
import QtQuick.Controls 2.1

TextField {
    id: control

    height: 50

    font.family: "Graphik Regular"
    font.pointSize: 10.5
    color: (text.length == 0 || !enabled) ? "#7e8c81" : "#000000"

    property bool togglePasswordField: false
    verticalAlignment: TextInput.AlignVCenter
    horizontalAlignment: TextInput.AlignLeft
    placeholderText: "PlaceHolder"
    property string seperatorColor: "#FFFFFF"
    selectByMouse: true

    background: Item {
        anchors.fill: parent
        Rectangle {
            id: background
            color: "#FFFFFF"
            anchors.fill: parent

            Image {
                id: hide
                visible: control.togglePasswordField
                source: (control.echoMode == TextInput.Password) ? PathUtils.resourcePath("images/showPass.png") :
                    PathUtils.resourcePath("images/hidePass.png");
                fillMode: Image.PreserveAspectFit 
                width: 24
                smooth: true
                anchors {
                    top: parent.top
                    topMargin: 18
                    bottom: parent.bottom
                    bottomMargin: 18
                    right: parent.right
                    rightMargin: 13
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (control.echoMode === TextInput.Password) {
                            control.echoMode = TextInput.Normal;
                        } else {
                            control.echoMode = TextInput.Password;
                        }
                    }
                }
            }
        }
    }
}
