import QtQuick 2.3
import QtQuick.Controls 2.1

TextField {
    id: control
    //color: "#000000"
    font.family: "Graphik Medium"
    font.pixelSize: 22
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
                width: 21
                height: 14
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
