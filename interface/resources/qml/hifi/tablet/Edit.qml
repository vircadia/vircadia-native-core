import QtQuick 2.5
import QtQuick.Controls 1.0
import QtWebEngine 1.1
import QtWebChannel 1.0
import "../../controls"
import HFWebEngineProfile 1.0

StackView {
    id: editRoot
    objectName: "stack"
    initialItem: editBasePage

    property var eventBridge;
    signal sendToScript(var message);

    function pushSource(path) {
        editRoot.push(Qt.resolvedUrl(path));
        editRoot.currentItem.eventBridge = editRoot.eventBridge;
        editRoot.currentItem.sendToScript.connect(editRoot.sendToScript);
    }

    function popSource() {
        editRoot.pop();
    }

    Component {
        id: editBasePage
        TabView {
            id: editTabView
            anchors.fill: parent

            Tab {
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
}
