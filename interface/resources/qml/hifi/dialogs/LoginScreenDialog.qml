//
//  LoginScreenDialog.qml
//
//  Created by Wayne Chen on 2018/10/9
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles
import "qrc:///controls-uit" as HifiControlsUit
import "qrc:///styles-uit" as HifiStylesUit


LoginDialog {
    id: loginDialog

    Loader {
        id: bodyLoader
        Item {

            function login() {
                flavorText.visible = false
                mainTextContainer.visible = false
                toggleLoading(true)
                loginDialog.login(usernameField.text, passwordField.text)
            }

            Connections {
                target: loginScreenDialog
                onHandleLoginCompleted: {
                    console.log("Login Succeeded, linking steam account")
                    var poppedUp = Settings.getValue("loginDialogPoppedUp", false);
                    if (poppedUp) {
                        console.log("[ENCOURAGELOGINDIALOG]: logging in")
                        var data = {
                            "action": "user logged in"
                        };
                        UserActivityLogger.logAction("encourageLoginDialog", data);
                        Settings.setValue("loginDialogPoppedUp", false);
                    }
                    if (loginDialog.isSteamRunning()) {
                        loginDialog.linkSteam()
                    } else {
                        bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
                        bodyLoader.item.width = root.pane.width
                        bodyLoader.item.height = root.pane.height
                    }
                }
                onHandleLoginFailed: {
                    console.log("Login Failed")
                    var poppedUp = Settings.getValue("loginDialogPoppedUp", false);
                    if (poppedUp) {
                        console.log("[ENCOURAGELOGINDIALOG]: failed logging in")
                        var data = {
                            "action": "user failed logging in"
                        };
                        UserActivityLogger.logAction("encourageLoginDialog", data);
                        Settings.setValue("loginDialogPoppedUp", false);
                    }
                    flavorText.visible = true
                    mainTextContainer.visible = true
                    toggleLoading(false)
                }
                onHandleLinkCompleted: {
                    console.log("Link Succeeded")

                    bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
                    bodyLoader.item.width = root.pane.width
                    bodyLoader.item.height = root.pane.height
                }
                onHandleLinkFailed: {
                    console.log("Link Failed")
                    toggleLoading(false)
                }
            }

        }
    }
}
