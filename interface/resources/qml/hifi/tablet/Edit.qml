import QtQuick 2.5
import QtQuick.Controls 1.0
import QtWebEngine 1.1
import QtWebChannel 1.0
import "../../controls"
import HFWebEngineProfile 1.0

Item {
    id: editRoot
    property var eventBridge;

    TabView {
        id: editTabView
        anchors.fill: parent

        Tab {
            title: "Entity List"
            active: true
            enabled: true
            property string originalUrl: ""

            WebView {
                id: entityListToolWebView
                url: "../../../../../scripts/system/html/entityList.html"
                eventBridge: editRoot.eventBridge
                anchors.fill: parent
                enabled: true
            }
        }

        Tab {
            title: "Entity Properties"
            active: true
            enabled: true
            property string originalUrl: ""

            WebView {
                id: entityPropertiesWebView
                url: "../../../../../scripts/system/html/entityProperties.html"
                eventBridge: editRoot.eventBridge
                anchors.fill: parent
                enabled: true
            }
        }

        Tab {
            title: "Grid"
            active: true
            enabled: true
            property string originalUrl: ""

            WebView {
                id: entityPropertiesWebView
                url: "../../../../../scripts/system/html/gridControls.html"
                eventBridge: editRoot.eventBridge
                anchors.fill: parent
                enabled: true
            }
        }
    }
}
