import QtQuick 2.7
import controlsUit 1.0 as HiFiControls
import "../styles" as HifiStyles
import stylesUit 1.0

Item {
    id: root
    HifiConstants { id: hifi }
    width: parent !== null ? parent.width : undefined
    height: parent !== null ? parent.height : undefined
    property var parentStackItem: null
    property int headerHeight: 70
    property string scriptURL
    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    onKeyboardRaisedChanged: {
        if(!keyboardRaised) {
            webroot.unfocus();
        } else {
            webroot.stopUnfocus();
        }
    }
    property bool punctuationMode: false
    property bool passwordField: false
    property bool isDesktop: false
    property alias url: web.url
    property alias webView: web.webViewCore
    // FIXME - Reimplement profiles for... why? Was it so that new windows opened share the same profile? 
    //         Are profiles written to by the webengine during the session?
    //         Removed in PR Feature/web entity user agent #988
    //
    // property alias profile: web.webViewCoreProfile
    property bool remove: false
    property bool closeButtonVisible: true

    // Manage own browse history because WebEngineView history is wiped when a new URL is loaded via
    // onNewViewRequested, e.g., as happens when a social media share button is clicked.
    property var history: []
    property int historyIndex: -1

    Rectangle {
        id: buttons
        width: parent.width
        height: parent.headerHeight
        color: hifi.colors.white

        Row {
            id: nav
            anchors {
                top: parent.top
                topMargin: 10
                horizontalCenter: parent.horizontalCenter
            }
            spacing: 120

            TabletWebButton {
                id: back
                enabledColor: hifi.colors.darkGray
                disabledColor: hifi.colors.lightGrayText
                enabled: true
                text: "BACK"

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (historyIndex > 0) {
                            goBack();
                        } else {
                            closeWebEngine();
                        }
                    }
                }
            }

            TabletWebButton {
                id: close
                enabledColor: hifi.colors.darkGray
                disabledColor: hifi.colors.lightGrayText
                enabled: true
                text: "CLOSE"
                visible: closeButtonVisible

                MouseArea {
                    anchors.fill: parent
                    onClicked: closeWebEngine()
                }
            }
        }

        RalewaySemiBold {
            id: displayUrl
            color: hifi.colors.baseGray
            font.pixelSize: 12
            verticalAlignment: Text.AlignLeft
            text: web.url
            anchors {
                top: nav.bottom
                horizontalCenter: parent.horizontalCenter;
                left: parent.left
                leftMargin: 20
            }
        }

        MouseArea {
            anchors.fill: parent
            preventStealing: true
            propagateComposedEvents: true
        }
    }

    function goBack() {
        if (historyIndex > 0) {
            historyIndex--;
            loadUrl(history[historyIndex]);
        }
    }

    function closeWebEngine() {
        if (remove) {
            root.destroy();
            return;
        }
        if (parentStackItem) {
            parentStackItem.pop();
        } else {
            root.visible = false;
        }
    }

    function goForward() {
        if (historyIndex < history.length - 1) {
            historyIndex++;
            loadUrl(history[historyIndex]);
        }
    }

    function reloadPage() {
        view.reloadAndBypassCache()
        view.setActiveFocusOnPress(true);
        view.setEnabled(true);
    }

    function loadUrl(url) {
        web.webViewCore.url = url
    }

    Rectangle {
        anchors.fill: web;
        color: hifi.colors.white;
    }

    FlickableWebViewCore {
        id: web
        width: parent.width
        height: keyboardEnabled && keyboardRaised ? parent.height - keyboard.height - root.headerHeight : parent.height - root.headerHeight
        anchors.top: buttons.bottom

        onUrlChanged: {
            // Record history, skipping null and duplicate items.
            var urlString = url + "";
            urlString = urlString.replace(/\//g, "%2F");  // Consistent representation of "/"s to avoid false differences.
            if (urlString.length > 0 && (historyIndex === -1 || urlString !== history[historyIndex])) {
                historyIndex++;
                history = history.slice(0, historyIndex);
                history.push(urlString);
            }
        }

        onLoadingChangedCallback: {
            keyboardRaised = false;
            punctuationMode = false;
            keyboard.resetShiftMode(false);
            webViewCore.forceActiveFocus();
        }

        onNewViewRequestedCallback: {
            request.openIn(webViewCore);
        }
    }

    HiFiControls.Keyboard {
        id: keyboard
        raised: parent.keyboardEnabled && parent.keyboardRaised
        numeric: parent.punctuationMode
        password: parent.passwordField

        onPasswordChanged: {
            keyboard.mirroredText = "";
        }

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }

    Component.onCompleted: {
        root.isDesktop = (typeof desktop !== "undefined");
        keyboardEnabled = HMD.active;
    }

    Component.onDestruction: {
        keyboardRaised = false;
    }

    Keys.onPressed: {
        switch(event.key) {
        case Qt.Key_L:
            if (event.modifiers == Qt.ControlModifier) {
                event.accepted = true
            }
            break;
        }
    }
}
