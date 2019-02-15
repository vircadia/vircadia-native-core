import QtQuick 2.0

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Item {
    id: root

    visible: false

    property var avatarDoctor: null
    property var errors: []

    property int minimumDiagnoseTimeMS: 1000

    signal doneDiagnosing

    onVisibleChanged: {
        if (root.avatarDoctor !== null) {
            root.avatarDoctor.complete.disconnect(_private.avatarDoctorComplete);
            root.avatarDoctor = null;
        }
        if (doneTimer.running) {
            doneTimer.stop();
        }

        if (!root.visible) {
            return;
        }

        root.avatarDoctor = AvatarPackagerCore.currentAvatarProject.diagnose();
        root.avatarDoctor.complete.connect(this, _private.avatarDoctorComplete);
        _private.startTime = Date.now();
        root.avatarDoctor.startDiagnosing();
    }

    QtObject {
        id: _private
        property real startTime: 0

        function avatarDoctorComplete(errors) {
            if (!root.visible) {
                return;
            }

            console.warn("avatarDoctor.complete " + JSON.stringify(errors));
            root.errors = errors;
            AvatarPackagerCore.currentAvatarProject.hasErrors = errors.length > 0;
            AvatarPackagerCore.addCurrentProjectToRecentProjects();

            let timeSpendDiagnosingMS = Date.now() - _private.startTime;
            let timeLeftMS = root.minimumDiagnoseTimeMS - timeSpendDiagnosingMS;
            doneTimer.interval = timeLeftMS < 0 ? 0 : timeLeftMS;
            doneTimer.start();
        }
    }

    Timer {
        id: doneTimer
        repeat: false
        running: false
        onTriggered: {
            doneDiagnosing();
        }
    }

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
