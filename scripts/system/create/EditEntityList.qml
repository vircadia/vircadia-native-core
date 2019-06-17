import QtQuick 2.7
import QtQuick.Controls 2.2
import QtWebChannel 1.0
import "qrc:////qml//controls"
import "qrc:////qml//hifi//toolbars"
import QtGraphicalEffects 1.0
import controlsUit 1.0 as HifiControls
import stylesUit 1.0


WebView {
    id: entityListToolWebView
    url: Paths.defaultScripts + "/system/html/entityList.html"
    enabled: true
    blurOnCtrlShift: false
}
