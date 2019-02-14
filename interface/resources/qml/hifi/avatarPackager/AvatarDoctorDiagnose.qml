import QtQuick 2.0

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Item {
    id: diagnosingScreen

    visible: false

    property var avatarDoctor: null
    property var errors: []

    signal doneDiagnosing

    onVisibleChanged: {
        if (!diagnosingScreen.visible) {
            //if (debugDelay.running) {
            //    debugDelay.stop();
            //}
            return;
        }
        //debugDelay.start();
        avatarDoctor = AvatarPackagerCore.currentAvatarProject.diagnose();
        avatarDoctor.complete.connect(function(errors) {
            console.warn("avatarDoctor.complete " + JSON.stringify(errors));
            diagnosingScreen.errors = errors;
            AvatarPackagerCore.currentAvatarProject.hasErrors = errors.length > 0;
            AvatarPackagerCore.addCurrentProjectToRecentProjects();

            // FIXME: can't seem to change state here so do it with a timer instead
            doneTimer.start();
        });
        avatarDoctor.startDiagnosing();
    }

    Timer {
        id: doneTimer
        interval: 1
        repeat: false
        running: false
        onTriggered: {
            doneDiagnosing();
        }
    }

/*
    Timer {
        id: debugDelay
        interval: 5000
        repeat: false
        running: false
        onTriggered: {
            if (Math.random() > 0.5) {
                // ERROR
                avatarPackager.state = AvatarPackagerState.avatarDoctorErrorReport;
            } else {
                // SUCCESS
                avatarPackager.state = AvatarPackagerState.project;
            }
        }
    }
*/

    property var footer: Item {
        anchors.fill: parent
        anchors.rightMargin: 17
        HifiControls.Button {
            id: cancelButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            height: 30
            width: 133
            text: qsTr("Cancel")
            onClicked: {
                avatarPackager.state = AvatarPackagerState.main;
            }
        }
    }

    LoadingCircle {
        id: loadingCircle
        anchors {
            top: parent.top
            topMargin: 46
            horizontalCenter: parent.horizontalCenter
        }
        width: 163
        height: 163
    }

    RalewayRegular {
        id: testingPackageTitle

        anchors {
            horizontalCenter:  parent.horizontalCenter
            top: loadingCircle.bottom
            topMargin: 5
        }

        text: "Testing package for errors"
        size: 28
        color: "white"
    }

    RalewayRegular {
        id: testingPackageText

        anchors {
            top: testingPackageTitle.bottom
            topMargin: 26
            left: parent.left
            leftMargin: 21
            right: parent.right
            rightMargin: 16
        }

        text: "We are trying to find errors in your project so you can quickly understand and resolve them."
        size: 21
        color: "white"
        lineHeight: 33
        lineHeightMode: Text.FixedHeight
        wrapMode: Text.Wrap
    }
}
