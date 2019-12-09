//
//  ScriptWhitelist.qml
//  interface/resources/qml/hifi/dialogs/security
//
//  Created by Kasen IO on 2019.12.05 | realities.dev | kasenvr@gmail.com
//  Copyright 2019 Kasen IO
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// Security Settings for the Entity Script Whitelist

import Hifi 1.0 as Hifi
import QtQuick 2.8
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.12
import stylesUit 1.0 as HifiStylesUit
import controlsUit 1.0 as HiFiControls
import PerformanceEnums 1.0
import "../../../windows"


Rectangle {
  
  function getWhitelistAsText() {
    var whitelist = Settings.getValue("private/settingsSafeURLS");
    return whitelist;
  }
  
  function setWhiteListAsText(whitelistText) {
    console.info('SETTINGCURRENTVALUE: ', Settings.getValue('private/settingsSafeURLS'), whitelistText.text)
    Settings.setValue("private/settingsSafeURLS", whitelistText.text);
    console.info('SETVALUE: ', Settings.getValue('private/settingsSafeURLS'), whitelistText.text)
  }
  
  anchors.fill: parent
  width: parent.width;
  height: 120;
  color: "#80010203";

  HifiStylesUit.RalewayRegular {
      id: titleText;
      text: "Entity Script Whitelist"
      // Text size
      size: 24;
      // Style
      color: "white";
      elide: Text.ElideRight;
      // Anchors
      anchors.top: parent.top;
      anchors.left: parent.left;
      anchors.leftMargin: 20;
      anchors.right: parent.right;
      anchors.rightMargin: 20;
      height: 60;
  }
  
  Rectangle {
    id: textAreaRectangle;
    color: "black";
    width: parent.width;
    height: 250;
    anchors.top: titleText.bottom;
    
    ScrollView {
      id: textAreaScrollView
      anchors.fill: parent;
      width: parent.width
      height: parent.height
      contentWidth: parent.width
      contentHeight: parent.height
      clip: false;
      
      TextArea {
        id: whitelistTextArea
        text: getWhitelistAsText();
        width: parent.width;
        height: parent.height;
        font.family: "Ubuntu";
        font.pointSize: 12;
        color: "white";
      }
    }
    
    Button {
      id: saveChanges
      anchors.topMargin: 5;
      anchors.leftMargin: 20;
      anchors.rightMargin: 20;
      x: textAreaRectangle.x + textAreaRectangle.width - width - 5;
      y: textAreaRectangle.y + textAreaRectangle.height - height;
      contentItem: Text {
        text: saveChanges.text
        font.family: "Ubuntu";
        font.pointSize: 12;
        opacity: enabled ? 1.0 : 0.3
        color: "black"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
      }
      text: "Save Changes"
      onClicked: setWhiteListAsText(whitelistTextArea)
    }
  }
}