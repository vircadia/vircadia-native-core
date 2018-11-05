import QtQuick 2.7
import QtQuick.Controls 2.2
import QtWebChannel 1.0
import "../../controls"
import "../toolbars"
import QtGraphicalEffects 1.0
import "../../controls-uit" as HifiControls
import "../../styles-uit"


WebView {
    id: entityListToolWebView
    url: Paths.defaultScripts + "/system/html/entityList.html"
    enabled: true
}
