import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls.Styles 1.3
import "controls"
import "styles"

DialogContainer {
    HifiConstants { id: hifi }
    id: root
    objectName: "UpdateDialog"
    implicitWidth: updateDialog.width
    implicitHeight: updateDialog.height
    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0
    
    UpdateDialog {
        id: updateDialog
        
        implicitWidth: backgroundRectangle.width
        implicitHeight: backgroundRectangle.height
        
        readonly property int inputWidth: 500
        readonly property int inputHeight: 60
        readonly property int borderWidth: 30
        readonly property int closeMargin: 16
        readonly property int inputSpacing: 16
        
        Column {
            id: mainContent
            width: updateDialog.inputWidth
            spacing: updateDialog.inputSpacing
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }
            
            Rectangle {
                id: backgroundRectangle
                color: "#2c86b1"
                opacity: 0.85
                radius: updateDialog.closeMargin * 2
                
                width: updateDialog.inputWidth + updateDialog.borderWidth * 2
                height: updateDialog.inputHeight * 6 + updateDialog.closeMargin * 2
                
                Rectangle {
                    id: dialogTitle
                    width: updateDialog.inputWidth
                    height: updateDialog.inputHeight
                    radius: height / 2
                    color: "#ebebeb"
                    
                    anchors {
                        top: parent.top
                        topMargin: updateDialog.inputSpacing
                        horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        id: updateAvailableText
                        text: "Update Available"
                        anchors {
                            verticalCenter: parent.verticalCenter
                            left: parent.left
                            leftMargin: updateDialog.inputSpacing
                        }
                    }
                    
                    Text {
                        text: updateDialog.updateAvailableDetails
                        font.pixelSize: 14
                        color: hifi.colors.text
                        anchors {
                            verticalCenter: parent.verticalCenter
                            left: updateAvailableText.right
                            leftMargin: 13
                        }
                    }
                }
                
                Flickable {
                    id: scrollArea
                    anchors {
                        top: dialogTitle.bottom
                    }
                    contentWidth: updateDialog.inputWidth
                    contentHeight: backgroundRectangle.height - (dialogTitle.height * 2)
                    width: updateDialog.inputWidth
                    height: backgroundRectangle.height - (dialogTitle.height * 2)
                    flickableDirection: Flickable.VerticalFlick
                    clip: true
                    
                    TextEdit {
                        id: releaseNotes
                        wrapMode: TextEdit.Wrap
                        width: parent.width
                        readOnly: true
                        text: updateDialog.releaseNotes
                        font.pixelSize: 14
                        color: hifi.colors.text
                        anchors {
                            left: parent.left
                            leftMargin: updateDialog.borderWidth
                        }
                    }
                    
                }
            }
        }
    }
}