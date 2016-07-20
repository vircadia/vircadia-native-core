Window {
    id: root
    HifiConstants { id: hifi }

    width: 550
    height: 200

    anchors.centerIn: parent
    resizable: true

    property bool required: false

    Component {
        id: welcomeBody

        Column {
            anchors.centerIn: parent

            OverlayTitle {
                anchors.horizontalCenter: parent.horizontalCenter

                text: "Welcomeback Atlante45!"
                color: hifi.colors.baseGrayHighlight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }

            VerticalSpacer {}

            HorizontalRule {}

            MenuItem {
                id: details
                anchors.horizontalCenter: parent.horizontalCenter

                text: "You are now signed into High Fidelity"
                color: hifi.colors.baseGrayHighlight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }

            VerticalSpacer {}

            Button {
                anchors.horizontalCenter: parent.horizontalCenter

                text: "Close"
            }
        }
    }

    Component {
        id: signInBody

        Column {
          anchors.centerIn: parent

          OverlayTitle {
            anchors.horizontalCenter: parent.horizontalCenter

            text: required ? "Sign In Required" : "Sign In"
            color: hifi.colors.baseGrayHighlight
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
          }

          VerticalSpacer {}

          HorizontalRule {}

          MenuItem {
            id: details
            anchors.horizontalCenter: parent.horizontalCenter

            text: required ? "This domain's owner requires that you sign in:"
                           : "Sign in to access your user account:"
            color: hifi.colors.baseGrayHighlight
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
          }

          VerticalSpacer {}

          Row {
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
              anchors.verticalCenter: parent.verticalCenter
              width: undefined // invalidate so that the image's size sets the width
              height: undefined // invalidate so that the image's size sets the height

              style: Original.ButtonStyle {
                background: Image {
                    id: buttonImage
                    source: "../images/steam-sign-in.png"
                }
              }

              onClicked: body.sourceComponent = completeProfileBody
            }

            HorizontalSpacer {}

            Button {
              anchors.verticalCenter: parent.verticalCenter

              text: "Cancel"

              onClicked: required = !required
            }
          }
        }
    }

    Component {
        id: completeProfileBody

        Column {
            anchors.centerIn: parent

            Row {
                anchors.horizontalCenter: parent.horizontalCenter

                HiFiGlyphs {
                    anchors.verticalCenter: parent.verticalCenter

                    text: hifi.glyphs.avatar
                }

                OverlayTitle {
                    anchors.verticalCenter: parent.verticalCenter

                    text: "Complete Your Profile"
                    color: hifi.colors.baseGrayHighlight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            VerticalSpacer {}

            HorizontalRule {}

            VerticalSpacer {}

            Row {
                anchors.horizontalCenter: parent.horizontalCenter

                Button {
                  anchors.verticalCenter: parent.verticalCenter

                  width: 200

                  text: "Create your profile"
                  color: hifi.buttons.blue

                  onClicked: body.sourceComponent = welcomeBody
                }

                HorizontalSpacer {}

                Button {
                  anchors.verticalCenter: parent.verticalCenter

                  text: "Cancel"

                  onClicked: body.sourceComponent = signInBody

                }
            }

            VerticalSpacer {}

            ShortcutText {
                text: "Already have a High Fidelity profile? Link to an existing profile here."

                color: hifi.colors.blueAccent
                font.underline: true

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Rectangle {
      anchors.fill: root
      color: hifi.colors.faintGray
      radius: hifi.dimensions.borderRadius

      Loader {
        id: body
        anchors.centerIn: parent
        sourceComponent: signInBody
      }
    }
}
