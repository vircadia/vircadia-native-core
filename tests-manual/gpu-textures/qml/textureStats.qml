import QtQuick 2.5
import QtQuick.Controls 2.3

Item {
    width: 400
    height: 600

    Column {
        spacing: 10
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10

        Text { text: qsTr("Total") }
        Text { text: Stats.total + " MB" }
        Text { text: qsTr("Allocated") }
        Text { text: Stats.allocated }
        Text { text: qsTr("Populated") }
        Text { text: Stats.populated }
        Text { text: qsTr("Pending") }
        Text { text: Stats.pending }
        Text { text: qsTr("Current Index") }
        Text { text: Stats.index }
        Text { text: qsTr("Current Source") }
        Text { text: Stats.source }
        Text { text: qsTr("Current Rez") }
        Text { text: Stats.rez.width + " x " + Stats.rez.height }
    }

    Row {
        id: row1
        spacing: 10
        anchors.bottom: row2.top
        anchors.left: parent.left
        anchors.margins: 10
        Button { text: "1024"; onClicked: Stats.maxTextureMemory(1024); }
        Button { text: "256"; onClicked: Stats.maxTextureMemory(256); }
    }

    Row {
        id: row2
        spacing: 10
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 10
        Button { text: "Change Textures"; onClicked: Stats.changeTextures(); }
        Button { text: "Next"; onClicked: Stats.nextTexture(); }
        Button { text: "Previous"; onClicked: Stats.prevTexture(); }
    }

}

