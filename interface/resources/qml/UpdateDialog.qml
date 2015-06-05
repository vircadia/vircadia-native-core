import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls.Styles 1.3
import "controls"
import "styles"

UpdateDialog {
    HifiConstants { id: hifi }
    id: updateDialog
    objectName: "UpdateDialog"
    implicitWidth: backgroundImage.width
    implicitHeight: backgroundImage.height + releaseNotes.height
    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0
    
    Image {
        id: backgroundImage
        source: "../images/update-available.svg"
        width: 576
        height: 80
        smooth: true
        
        Text {
            id: updateAvailableTitle
            text: "Update available"
            color: "#000000"
            x: 90
            y: 17
        }
        
        Text {
            id: updateAvailableDetails
            text: updateDialog.updateAvailableDetails
            width: parent.width
            anchors.top: updateAvailableTitle.bottom
            anchors.topMargin: 3
            font.pixelSize: 15
            color: "#545454"
            x: 90
        }
    }
    
    Rectangle {
        id: releaseNotes
        color: "#CCE8E8E8"
        anchors.top: backgroundImage.bottom
        anchors.topMargin: 0
        width: backgroundImage.width - 90
        height: releaseNotesContent.height
        x: 50
        radius: 10
        
        
        TextEdit {
            id: releaseNotesContent
            readOnly: true
            font.pixelSize: 13
            width: parent.width
            x: 10
            y: 10
            color: "#111111"
            text:
                "These are the release notes: \n" +
                "And some more text about what this includes: \n" +
                "These are the release notes: \n" +
                "And some more text about what this includes: \n" +
                "These are the release notes: \n" +
                "And some more text about what this includes: \n";
        }
    }
    
}