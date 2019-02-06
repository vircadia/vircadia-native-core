import QtQuick 2.5

import controlsUit 1.0 as HifiControls
import stylesUit 1.0

Item {
    id: root
    HifiConstants { id: hifi }

    height: 600
    property variant permissionsBar: {'securityOrigin':'none','feature':'none'}
    property alias url: webview.url

    property bool canGoBack: webview.canGoBack
    property bool canGoForward: webview.canGoForward


    signal loadingChanged(int status)

    x: 0
    y: 0

    function setProfile(profile) {
        webview.profile = profile;
    }

    HifiControls.ProxyWebView {
        id: webview
        width: parent.width
        height: parent.height
    }
}
