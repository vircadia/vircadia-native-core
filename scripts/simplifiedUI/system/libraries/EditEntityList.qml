import QtQuick 2.7
import QtQuick.Controls 2.2
import QtWebChannel 1.0
import QtGraphicalEffects 1.0
import "qrc:///qml/controls" as HifiControls

HifiControls.WebView {
    id: entityListToolWebView
    url: Qt.resolvedUrl("../html/entityList.html")
    enabled: true
    blurOnCtrlShift: false
}
