//
//  SignUpBody.qml
//
//  Created by Stephen Birarda on 7 Dec 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import "../controls-uit"
import "../styles-uit"

Item {
  id: signupBody
  clip: true
  width: root.pane.width
  height: root.pane.height

  function signup() {
      mainTextContainer.visible = false
      loginDialog.signup(usernameField.text, passwordField.text)
  }

  property bool keyboardEnabled: false
  property bool keyboardRaised: false
  property bool punctuationMode: false

  onKeyboardRaisedChanged: d.resize();

  QtObject {
      id: d
      readonly property int minWidth: 480
      readonly property int maxWidth: 1280
      readonly property int minHeight: 120
      readonly property int maxHeight: 720

      function resize() {
          var targetWidth = Math.max(titleWidth, form.contentWidth);
          var targetHeight =  hifi.dimensions.contentSpacing.y + mainTextContainer.height +
                          4 * hifi.dimensions.contentSpacing.y + form.height +
                              hifi.dimensions.contentSpacing.y + buttons.height;

          root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
          root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
                  + (keyboardEnabled && keyboardRaised ? (200 + 2 * hifi.dimensions.contentSpacing.y) : hifi.dimensions.contentSpacing.y);
      }
  }

  ShortcutText {
      id: mainTextContainer
      anchors {
          top: parent.top
          left: parent.left
          margins: 0
          topMargin: hifi.dimensions.contentSpacing.y
      }

      visible: false

      text: qsTr("Username or password incorrect.")
      wrapMode: Text.WordWrap
      color: hifi.colors.redAccent
      lineHeight: 1
      lineHeightMode: Text.ProportionalHeight
      horizontalAlignment: Text.AlignHCenter
  }

  Column {
      id: form
      anchors {
          top: mainTextContainer.bottom
          left: parent.left
          margins: 0
          topMargin: 2 * hifi.dimensions.contentSpacing.y
      }
      spacing: 2 * hifi.dimensions.contentSpacing.y

      Row {
          spacing: hifi.dimensions.contentSpacing.x

          TextField {
              id: usernameField
              anchors {
                  verticalCenter: parent.verticalCenter
              }
              width: 350

              label: "Username"
          }

          ShortcutText {
              anchors {
                  verticalCenter: parent.verticalCenter
              }

              wrapMode: Text.WordWrap
              lineHeight: 1
              lineHeightMode: Text.ProportionalHeight

              text: qsTr("Must be unique. No spaces or other special characters")

              verticalAlignment: Text.AlignVCenter
              horizontalAlignment: Text.AlignHCenter

              color: hifi.colors.blueAccent
          }
      }
      Row {
          spacing: hifi.dimensions.contentSpacing.x

          TextField {
              id: passwordField
              anchors {
                  verticalCenter: parent.verticalCenter
              }
              width: 350

              label: "Password"
              echoMode: TextInput.Password
          }

          ShortcutText {
              anchors {
                  verticalCenter: parent.verticalCenter
              }

              text: qsTr("At least 6 characters")

              verticalAlignment: Text.AlignVCenter
              horizontalAlignment: Text.AlignHCenter

              color: hifi.colors.blueAccent
          }
      }

  }

  // Override ScrollingWindow's keyboard that would be at very bottom of dialog.
  Keyboard {
      raised: keyboardEnabled && keyboardRaised
      numeric: punctuationMode
      anchors {
          left: parent.left
          right: parent.right
          bottom: buttons.top
          bottomMargin: keyboardRaised ? 2 * hifi.dimensions.contentSpacing.y : 0
      }
  }

  Row {
        id: leftButton
        anchors {
            left: parent.left
            bottom: parent.bottom
            bottomMargin: hifi.dimensions.contentSpacing.y
        }

        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Existing User")

            onClicked: {
              bodyLoader.setSource("LinkAccountBody.qml")
              bodyLoader.item.width = root.pane.width
              bodyLoader.item.height = root.pane.height
            }
        }
    }

  Row {
      id: buttons
      anchors {
          right: parent.right
          bottom: parent.bottom
          bottomMargin: hifi.dimensions.contentSpacing.y
      }
      spacing: hifi.dimensions.contentSpacing.x
      onHeightChanged: d.resize(); onWidthChanged: d.resize();

      Button {
          id: linkAccountButton
          anchors.verticalCenter: parent.verticalCenter
          width: 200

          text: qsTr("Sign Up")
          color: hifi.buttons.blue

          onClicked: signupBody.signup()
      }

      Button {
          anchors.verticalCenter: parent.verticalCenter

          text: qsTr("Cancel")

          onClicked: root.destroy()
      }
  }

  Component.onCompleted: {
      root.title = qsTr("Create an Account")
      root.iconText = "<"
      keyboardEnabled = HMD.active;
      d.resize();

      usernameField.forceActiveFocus();
  }

  Connections {
      target: loginDialog
      onHandleSignupCompleted: {
          console.log("Signup Succeeded")

          bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
          bodyLoader.item.width = root.pane.width
          bodyLoader.item.height = root.pane.height
      }
      onHandleSignupFailed: {
          console.log("Signup Failed")
          mainTextContainer.visible = true
      }
  }

  Keys.onPressed: {
      if (!visible) {
          return
      }

      switch (event.key) {
          case Qt.Key_Enter:
          case Qt.Key_Return:
              event.accepted = true
              signupBody.signup()
              break
      }
  }
}
