import QtQuick 2.7
import stylesUit 1.0

Rectangle {
    HifiConstants {
        id: hifi
    }

    color: hifi.colors.darkGray

    signal onNewViewRequested();

    property string url: "";
    property bool canGoBack: false
    property bool canGoForward: false
    property string icon: ""
    property var profile: {}

    property bool safeLoading: false
    property bool loadingLatched: false
    property var loadingRequest: null


    Text {
        anchors.centerIn: parent
        text: "This feature is not supported"
        font.pixelSize: 32
        color: hifi.colors.white
    }
}
