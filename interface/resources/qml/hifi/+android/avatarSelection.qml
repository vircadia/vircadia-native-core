//
//  avatarSelection.qml
//  interface/resources/qml/android
//
//  Created by Gabriel Calero & Cristian Duarte on 21 Sep 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Layouts 1.3
import Hifi 1.0

import "../../styles"
import "."
import ".."
import ".." as QmlHifi
import "../../styles-uit" as HifiStyles


Item {

    id: top

    HifiConstants { id: android }
    width: parent ? parent.width - android.dimen.windowLessWidth : 0
    height: parent ? parent.height - android.dimen.windowLessHeight : 0
    z: android.dimen.windowZ
    anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }

    signal sendToScript(var message);

    property bool shown: true

    onShownChanged: {
        top.visible = shown;
    }
    

    HifiConstants { id: hifi }
    HifiStyles.HifiConstants { id: hifiStyleConstants }

    property int cardWidth: 250 *3;
    property int cardHeight: 240 *3;
    property int gap: 14 *3;

    property var avatarsArray: [];
    property var extraOptionsArray: [];

    function hide() {
        shown = false;
        sendToScript ({ method: "hide" });
    }

    Rectangle {

        width: parent ? parent.width : 0
        height: parent ? parent.height : 0

        gradient: Gradient {
            GradientStop { position: 0.0; color: android.color.gradientTop }
            GradientStop { position: 1.0; color: android.color.gradientBottom }
        }

        QmlHifi.WindowHeader {
            id: header
            iconSource: "../../../../icons/avatar-i.svg"
            titleText: "AVATAR"
        }

        ListModel { id: avatars }

        ListView {
            id: scroll
            height: 250*3
            property int stackedCardShadowHeight: 10*3;
            spacing: gap;
            clip: true;
            anchors {
                left: parent.left
                right: parent.right
                top: header.bottom
                topMargin: gap
                leftMargin: gap
                rightMargin: gap
            }
            model: avatars;
            orientation: ListView.Horizontal;
            delegate: QmlHifi.AvatarOption {
                type: model.type;
                thumbnailUrl: model.thumbnailUrl;
                avatarUrl: model.avatarUrl;
                avatarName: model.avatarName;
                avatarSelected: model.avatarSelected;
                methodName: model.methodName;
                actionText: model.actionText;
            }
            highlightMoveDuration: -1;
            highlightMoveVelocity: -1;
        }

    }

    function escapeRegExp(str) {
        return str.replace(/([.*+?^=!:${}()|\[\]\/\\])/g, "\\$1");
    }
    function replaceAll(str, find, replace) {
        return str.replace(new RegExp(escapeRegExp(find), 'g'), replace);
    }

    function refreshSelected(selectedAvatarUrl) {
        // URL as ID?
        avatarsArray.forEach(function (avatarData) {
            avatarData.avatarSelected = (selectedAvatarUrl == avatarData.avatarUrl);
            console.log('[avatarSelection] avatar : ',  avatarData.avatarName, ' is selected? ' , avatarData.avatarSelected);
        });
    }

    function addAvatar(name, thumbnailUrl, avatarUrl) {
        avatarsArray.push({
            type: "avatar",
            thumbnailUrl: thumbnailUrl,
            avatarUrl: avatarUrl,
            avatarName: name,
            avatarSelected: false,
            methodName: "",
            actionText: ""
        });
    }

    function showAvatars() {
        avatars.clear();
        avatarsArray.forEach(function (avatarData) {
            avatars.append(avatarData);
            console.log('[avatarSelection] adding avatar to model: ', JSON.stringify(avatarData));
        });
        extraOptionsArray.forEach(function (extraData) {
            avatars.append(extraData);
            console.log('[avatarSelection] adding extra option to model: ', JSON.stringify(extraData));
        });
    }

    function addExtraOption(showName, thumbnailUrl, methodNameWhenClicked, actionText) {
        extraOptionsArray.push({
            type: "extra",
            thumbnailUrl: thumbnailUrl,
            avatarUrl: "",
            avatarName: showName,
            avatarSelected: false,
            methodName: methodNameWhenClicked,
            actionText: actionText
        });
    }

    function fromScript(message) {
        //console.log("[CHAT] fromScript " + JSON.stringify(message));
        switch (message.type) {
            case "addAvatar":
                addAvatar(message.name, message.thumbnailUrl, message.avatarUrl);
            break;
            case "addExtraOption":
                //(showName, thumbnailUrl, methodNameWhenClicked, actionText)
                addExtraOption(message.showName, message.thumbnailUrl, message.methodNameWhenClicked, message.actionText);
            break;
            case "refreshSelected":
                refreshSelected(message.selectedAvatarUrl);
            break;
            case "showAvatars":
                showAvatars();
            break;
            default:
        }
    }
}