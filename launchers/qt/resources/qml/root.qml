// root.qml

import QtQuick 2.3
import QtQuick.Controls 2.1
import HQLauncher 1.0
import "HFControls"

Item {
    id: root
    width: 627
    height: 540
    Loader {
        anchors.fill: parent
        id: loader


        function setBuildInfoState(state) {
            buildInfo.state = state;
        }

        function setStateInfoState(state) {
            stateInfo.state = state;
        }
    }

    Component.onCompleted: {
        loader.source = "./SplashScreen.qml";
        LauncherState.updateSourceUrl.connect(function(url) {
            loader.source = url;
        });
    }


    function loadPage(url) {
        loader.source = url;
    }

    HFTextRegular {
        id: stateInfo
        font.pixelSize: 12

        anchors.right: root.right
        anchors.top: root.top

        color: "#FFFFFF"
        text: LauncherState.uiState.toString() + " - " + LauncherState.applicationState

         states: [
             State {
                 name: "left"
                 AnchorChanges {
                     target: stateInfo
                     anchors.left: root.left
                     anchors.right: undefined
                 }
             },

             State {
                 name: "right"
                 AnchorChanges {
                     target: stateInfo
                     anchors.right: root.right
                     anchors.left: undefined
                 }
             }
         ]
    }

    HFTextRegular {
        id: buildInfo

        font.pixelSize: 12

        anchors {
            leftMargin: 10
            rightMargin: 10
            bottomMargin: 10

            right: root.right
            bottom: root.bottom
        }

        color: "#666"
        text: "V." + LauncherState.buildVersion;

        states: [
            State {
                name: "left"
                AnchorChanges {
                    target: buildInfo
                    anchors.left: root.left
                    anchors.right: undefined
                }
            },

            State {
                name: "right"
                AnchorChanges {
                    target: buildInfo
                    anchors.right: root.right
                    anchors.left: undefined
                }
            }
        ]
    }
}
