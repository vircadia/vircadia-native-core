import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1
import QtWebChannel 1.0
import QtQuick.Controls.Styles 1.4
import "../../controls"
import "../toolbars"
import QtGraphicalEffects 1.0
import "../../controls-uit" as HifiControls
import "../../styles-uit"


TabView {
    id: editTabView
    // anchors.fill: parent
    height: 60

    Tab {
        title: "CREATE"
        active: true
        enabled: true
        property string originalUrl: ""

        Rectangle {
            color: "#404040"

            Text {
                color: "#ffffff"
                text: "Choose an Entity Type to Create:"
                font.pixelSize: 14
                font.bold: true
                anchors.top: parent.top
                anchors.topMargin: 28
                anchors.left: parent.left
                anchors.leftMargin: 28
            }

            Flow {
                id: createEntitiesFlow
                spacing: 35
                anchors.right: parent.right
                anchors.rightMargin: 55
                anchors.left: parent.left
                anchors.leftMargin: 55
                anchors.top: parent.top
                anchors.topMargin: 70


                NewEntityButton {
                    icon: "icons/create-icons/94-model-01.svg"
                    text: "MODEL"
                    onClicked: {
                        editRoot.sendToScript({
                            method: "newEntityButtonClicked", params: { buttonName: "newModelButton" }
                        });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/21-cube-01.svg"
                    text: "CUBE"
                    onClicked: {
                        editRoot.sendToScript({
                            method: "newEntityButtonClicked", params: { buttonName: "newCubeButton" }
                        });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/22-sphere-01.svg"
                    text: "SPHERE"
                    onClicked: {
                        editRoot.sendToScript({
                            method: "newEntityButtonClicked", params: { buttonName: "newSphereButton" }
                        });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/24-light-01.svg"
                    text: "LIGHT"
                    onClicked: {
                        editRoot.sendToScript({
                            method: "newEntityButtonClicked", params: { buttonName: "newLightButton" }
                        });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/20-text-01.svg"
                    text: "TEXT"
                    onClicked: {
                        editRoot.sendToScript({
                            method: "newEntityButtonClicked", params: { buttonName: "newTextButton" }
                        });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/25-web-1-01.svg"
                    text: "WEB"
                    onClicked: {
                        editRoot.sendToScript({
                            method: "newEntityButtonClicked", params: { buttonName: "newWebButton" }
                        });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/23-zone-01.svg"
                    text: "ZONE"
                    onClicked: {
                        editRoot.sendToScript({
                            method: "newEntityButtonClicked", params: { buttonName: "newZoneButton" }
                        });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/90-particles-01.svg"
                    text: "PARTICLE"
                    onClicked: {
                        editRoot.sendToScript({
                            method: "newEntityButtonClicked", params: { buttonName: "newParticleButton" }
                        });
                        editTabView.currentIndex = 4
                    }
                }
            }

            HifiControls.Button {
                id: assetServerButton
                text: "Open This Domain's Asset Server"
                color: hifi.buttons.black
                colorScheme: hifi.colorSchemes.dark
                anchors.right: parent.right
                anchors.rightMargin: 55
                anchors.left: parent.left
                anchors.leftMargin: 55
                anchors.top: createEntitiesFlow.bottom
                anchors.topMargin: 35
                onClicked: {
                    editRoot.sendToScript({
                        method: "newEntityButtonClicked", params: { buttonName: "openAssetBrowserButton" }
                    });
                }
            }

            HifiControls.Button {
                text: "Import Entities (.json)"
                color: hifi.buttons.black
                colorScheme: hifi.colorSchemes.dark
                anchors.right: parent.right
                anchors.rightMargin: 55
                anchors.left: parent.left
                anchors.leftMargin: 55
                anchors.top: assetServerButton.bottom
                anchors.topMargin: 20
                onClicked: {
                    editRoot.sendToScript({
                        method: "newEntityButtonClicked", params: { buttonName: "importEntitiesButton" }
                    });
                }
            }
        }
    }

    Tab {
        title: "LIST"
        active: true
        enabled: true
        property string originalUrl: ""

        WebView {
            id: entityListToolWebView
            url: Paths.defaultScripts + "/system/html/entityList.html"
            anchors.fill: parent
            enabled: true
        }
    }

    Tab {
        title: "PROPERTIES"
        active: true
        enabled: true
        property string originalUrl: ""

        WebView {
            id: entityPropertiesWebView
            url: Paths.defaultScripts + "/system/html/entityProperties.html"
            anchors.fill: parent
            enabled: true
        }
    }

    Tab {
        title: "GRID"
        active: true
        enabled: true
        property string originalUrl: ""

        WebView {
            id: gridControlsWebView
            url: Paths.defaultScripts + "/system/html/gridControls.html"
            anchors.fill: parent
            enabled: true
        }
    }

    Tab {
        title: "P"
        active: true
        enabled: true
        property string originalUrl: ""

        WebView {
            id: particleExplorerWebView
            url: Paths.defaultScripts + "/system/particle_explorer/particleExplorer.html"
            anchors.fill: parent
            enabled: true
        }
    }


    style: TabViewStyle {
        frameOverlap: 1
        tab: Rectangle {
            color: styleData.selected ? "#404040" :"black"
            implicitWidth: text.width + 42
            implicitHeight: 40
            Text {
                id: text
                anchors.centerIn: parent
                text: styleData.title
                font.pixelSize: 16
                font.bold: true
                color: styleData.selected ? "white" : "white"
                property string glyphtext: ""
                HiFiGlyphs {
                    anchors.centerIn: parent
                    size: 30
                    color: "#ffffff"
                    text: text.glyphtext
                }
                Component.onCompleted: if (styleData.title == "P") {
                    text.text = "   ";
                    text.glyphtext = "\ue004";
                }
            }
        }
        tabBar: Rectangle {
            color: "black"
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.top: parent.top
            anchors.topMargin: 0
        }
    }

    function fromScript(message) {
        switch (message.method) {
            case 'selectTab':
                selectTab(message.params.id);
                break;
            default:
                console.warn('Unrecognized message:', JSON.stringify(message));
        }
    }

    // Changes the current tab based on tab index or title as input
    function selectTab(id) {
        if (typeof id === 'number') {
            if (id >= 0 && id <= 4) {
                editTabView.currentIndex = id;
            } else {
                console.warn('Attempt to switch to invalid tab:', id);
            }
        } else if (typeof id === 'string'){
            switch (id.toLowerCase()) {
                case 'create':
                    editTabView.currentIndex = 0;
                    break;
                case 'list':
                    editTabView.currentIndex = 1;
                    break;
                case 'properties':
                    editTabView.currentIndex = 2;
                    break;
                case 'grid':
                    editTabView.currentIndex = 3;
                    break;
                case 'particle':
                    editTabView.currentIndex = 4;
                    break;
                default:
                    console.warn('Attempt to switch to invalid tab:', id);
            }
        } else {
            console.warn('Attempt to switch tabs with invalid input:', JSON.stringify(id));
        }
    }
}
