import QtQuick 2.7

import QtQuick.Controls 2.2

import stylesUit 1.0 as StylesUIt
import controlsUit 1.0 as ControlsUit

Item {
    id: flick

    property alias url: webViewCore.url
    property alias canGoBack: webViewCore.canGoBack
    property alias webViewCore: webViewCore
    // FIXME - This was commented out to allow for manual setting of the userAgent.
    //
    // property alias webViewCoreProfile: webViewCore.profile

    property alias useBackground: webViewCore.useBackground
    property alias userAgent: webViewCore.userAgent
    property string userScriptUrl: ""
    property string urlTag: "noDownload=false"

    signal newViewRequestedCallback(var request)
    signal loadingChangedCallback(var loadRequest)


    width: parent.width

    property bool interactive: false

    property bool blurOnCtrlShift: true

    StylesUIt.HifiConstants {
        id: hifi
    }

    function stop() {

    }

    function unfocus() {
    }

    function stopUnfocus() {
    }

    function onLoadingChanged(loadRequest) {
    }

    ControlsUit.ProxyWebView {
        id: webViewCore
        width: parent.width
        height: parent.height
    }
}
