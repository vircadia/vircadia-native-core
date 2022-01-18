import QtQuick 2.7
import controlsUit 1.0 as HiFiControls

Item {
    id: root
    property alias url: webroot.url
    property alias scriptURL: webroot.userScriptUrl
    property alias canGoBack: webroot.canGoBack;
    property var goBack: webroot.webViewCore.goBack;
    property alias urlTag: webroot.urlTag
    property bool keyboardEnabled: true  // FIXME - Keyboard HMD only: Default to false
    property bool keyboardRaised: false
    onKeyboardRaisedChanged: {
        if(!keyboardRaised) {
            webroot.unfocus();
        } else {
            webroot.stopUnfocus();
        }
    }
    property bool punctuationMode: false

    // FIXME - Keyboard HMD only: Make Interface either set keyboardRaised property directly in OffscreenQmlSurface
    // or provide HMDinfo object to QML in RenderableWebEntityItem and do the following.
    /*
    onKeyboardRaisedChanged: {
        keyboardEnabled = HMDinfo.active;
    }
    */

    // FIXME - Reimplement profiles for... why? Was it so that new windows opened share the same profile? 
    //         Are profiles written to by the webengine during the session?
    //         Removed in PR Feature/web entity user agent #988
    //
    // property alias viewProfile: webroot.webViewCoreProfile

    FlickableWebViewCore {
        id: webroot
        width: parent.width
        height: keyboardEnabled && keyboardRaised ? parent.height - keyboard.height : parent.height

        onLoadingChangedCallback: {
            keyboardRaised = false;
            punctuationMode = false;
            keyboard.resetShiftMode(false);
        }

        onNewViewRequestedCallback: {
            // desktop is not defined for web-entities or tablet
            if (typeof desktop !== "undefined") {
                desktop.openBrowserWindow(request, webViewCoreProfile);
            } else {
                tabletRoot.openBrowserWindow(request, webViewCoreProfile);
            }
        }

    }

    HiFiControls.Keyboard {
        id: keyboard
        raised: parent.keyboardEnabled && parent.keyboardRaised
        numeric: parent.punctuationMode
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
