import QtQuick 2.5
import QtQuick.Controls 1.0
import QtWebEngine 1.1
import QtWebChannel 1.0
import QtQuick.Controls.Styles 1.4
import "../../controls"
import "../toolbars"
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
            // anchors.fill: parent
            height: 60

            Tab {
                title: "Create"
                active: true
                enabled: true
                property string originalUrl: ""

                Flow {
                    id: createEntitiesFlow
                    spacing: 16
                    anchors.right: parent.right
                    anchors.rightMargin: 30
                    anchors.left: parent.left
                    anchors.leftMargin: 30
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 30
                    anchors.top: parent.top
                    anchors.topMargin: 30

                    TabletButton {
                        icon: "icons/assets-01.svg"
                        text: "ASSETS"
                        onClicked: {
                            editRoot.sendToScript({
                                method: "newEntityButtonClicked", params: { buttonName: "openAssetBrowserButton" }
                            });
                        }
                    }

                    TabletButton {
                        icon: "model-01.svg"
                        text: "MODEL"
                        onClicked: {
                            editRoot.sendToScript({
                                method: "newEntityButtonClicked", params: { buttonName: "newModelButton" }
                            });
                        }
                    }

                    TabletButton {
                        icon: "../cube-01.svg"
                        text: "CUBE"
                        onClicked: {
                            editRoot.sendToScript({
                                method: "newEntityButtonClicked", params: { buttonName: "newCubeButton" }
                            });
                        }
                    }

                    ToolbarButton {
                        imageURL: "../images/sphere-01.svg"
                        onClicked: {
                            editRoot.sendToScript({
                                method: "newEntityButtonClicked", params: { buttonName: "newSphereButton" }
                            });
                        }
                    }

                    TabletButton {
                        icon: "../scripts/system/assets/images/tools/light-01.svg"
                        text: "LIGHT"
                        onClicked: {
                            editRoot.sendToScript({
                                method: "newEntityButtonClicked", params: { buttonName: "newLightButton" }
                            });
                        }
                    }

                    TabletButton {
                        icon: "../scripts/system/assets/images/tools/text-01.svg"
                        text: "TEXT"
                        onClicked: {
                            editRoot.sendToScript({
                                method: "newEntityButtonClicked", params: { buttonName: "newTextButton" }
                            });
                        }
                    }

                    TabletButton {
                        icon: "../scripts/system/assets/images/tools/web-01.svg"
                        text: "WEB"
                        onClicked: {
                            editRoot.sendToScript({
                                method: "newEntityButtonClicked", params: { buttonName: "newWebButton" }
                            });
                        }
                    }

                    TabletButton {
                        icon: "../scripts/system/assets/images/tools/zone-01.svg"
                        text: "ZONE"
                        onClicked: {
                            editRoot.sendToScript({
                                method: "newEntityButtonClicked", params: { buttonName: "newZoneButton" }
                            });
                        }
                    }

                    TabletButton {
                        icon: "../scripts/system/assets/images/tools/particle-01.svg"
                        text: "PARTICLE"
                        onClicked: {
                            editRoot.sendToScript({
                                method: "newEntityButtonClicked", params: { buttonName: "newParticleButton" }
                            });
                        }
                    }
                }
            }

            Tab {
                title: "List"
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
                title: "Properties"
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
                    id: gridControlsWebView
                    url: "../../../../../scripts/system/html/gridControls.html"
                    eventBridge: editRoot.eventBridge
                    anchors.fill: parent
                    enabled: true
                }
            }

            Tab {
                title: "Particles"
                active: true
                enabled: true
                property string originalUrl: ""

                WebView {
                    id: particleExplorerWebView
                    url: "../../../../../scripts/system/particle_explorer/particleExplorer.html"
                    eventBridge: editRoot.eventBridge
                    anchors.fill: parent
                    enabled: true
                }
            }


            style: TabViewStyle {
                frameOverlap: 1
                tab: Rectangle {
                    color: styleData.selected ? "slategrey" :"grey"
                    implicitWidth: text.width + 25
                    implicitHeight: 60
                    radius: 2
                    Text {
                        id: text
                        anchors.centerIn: parent
                        text: styleData.title
                        color: styleData.selected ? "white" : "white"
                    }
                }
            }
        }
    }
}
