import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1
import QtWebChannel 1.0
import QtQuick.Controls.Styles 1.4
import "../../../controls"
import "../../toolbars"
import HFWebEngineProfile 1.0
import QtGraphicalEffects 1.0
import "../../../controls-uit" as HifiControls
import "../../../styles-uit"

WebView {
    id: particleExplorerWebView
    url: "../../../../../../scripts/system/particle_explorer/particleExplorer.html"
    eventBridge: editRoot.eventBridge
    anchors.fill: parent
    enabled: true
}
