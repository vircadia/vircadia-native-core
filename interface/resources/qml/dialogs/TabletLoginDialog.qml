//
//  TabletLoginDialog.qml
//
//  Created by Vlad Stelmahovsky on 15 Mar 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.5
import QtQuick.Controls 1.4

import "../controls-uit"
import "../styles-uit"
import "../windows"

TabletModalWindow {
    id: loginDialogRoot
    objectName: "LoginDialog"

    signal sendToScript(var message);
    property bool isHMD: false
    property bool gotoPreviousApp: false;
    color: hifi.colors.baseGray

    property int colorScheme: hifi.colorSchemes.dark
    property int titleWidth: 0
    property string iconText: ""
    property int icon: hifi.icons.none
    property int iconSize: 35
    MouseArea {
        width: parent.width
        height: parent.height
    }

    property bool keyboardOverride: true
    onIconChanged: updateIcon();

    property var items;
    property string label: ""

    onTitleWidthChanged: d.resize();

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    onKeyboardRaisedChanged: d.resize();

    signal canceled();

    function updateIcon() {
        if (!root) {
            return;
        }
        iconText = hifi.glyphForIcon(root.icon);
    }

    property alias bodyLoader: bodyLoader
    property alias loginDialog: loginDialog
    property alias hifi: hifi

    HifiConstants { id: hifi }

    onCanceled: {
        if (loginDialogRoot.Stack.view) {
            loginDialogRoot.Stack.view.pop();
        } else if (gotoPreviousApp) {
            var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
            tablet.returnToPreviousApp();
        } else {
            Tablet.getTablet("com.highfidelity.interface.tablet.system").gotoHomeScreen();
        }
    }

    LoginDialog {
        id: loginDialog
        width: parent.width
        height: parent.height
        StackView {
            id: bodyLoader
            property var item: currentItem
            property var props
            property string source: ""

            onCurrentItemChanged: {
                //cleanup source for future usage
                source = ""
            }

            function setSource(src, props) {
                source = "../TabletLoginDialog/" + src
                bodyLoader.props = props
            }
            function popup() {
                bodyLoader.pop()

                //check if last screen, if yes, dialog is popped out
                if (depth === 1)
                    loginDialogRoot.canceled()
            }

            anchors.fill: parent
            anchors.margins: 10
            onSourceChanged: {
                if (source !== "") {
                    bodyLoader.push(Qt.resolvedUrl(source), props)
                }
            }
            Component.onCompleted: {
                setSource(loginDialog.isSteamRunning() ?
                              "SignInBody.qml" :
                              "LinkAccountBody.qml")
            }
        }
    }
}
