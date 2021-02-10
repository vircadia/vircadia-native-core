//
//  NameCard.qml
//  qml/hifi
//
//  Created by Howard Stearns on 12/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0
import stylesUit 1.0
import controlsUit 1.0 as HifiControls
import "toolbars"

// references Users, UserActivityLogger, MyAvatar, Vec3, Quat, AddressManager, Account from root context

Item {
    id: thisNameCard
    // Size
    width: isMyCard ? pal.myCardWidth - anchors.leftMargin : pal.nearbyNameCardWidth;
    height: isMyCard ? pal.myCardHeight : pal.rowHeight;
    anchors.left: parent.left
    anchors.leftMargin: 5
    anchors.top: parent.top;

    // Properties
    property string profileUrl: "";
    property string connectionStatus : ""
    property string uuid: ""
    property string displayName: ""
    property string userName: ""
    property real displayNameTextPixelSize: 18
    property int usernameTextPixelSize: 14
    property real audioLevel: 0.0
    property real avgAudioLevel: 0.0
    property bool isMyCard: false
    property bool selected: false
    property bool isAdmin: false
    property bool isPresent: true
    property bool isReplicated: false
    property string placeName: ""
    property string profilePicBorderColor: (connectionStatus == "connection" ? hifi.colors.indigoAccent : (connectionStatus == "friend" ? hifi.colors.greenHighlight : "transparent"))
    property alias avImage: avatarImage
    property bool has3DHTML: PlatformInfo.has3DHTML();

    Item {
        id: avatarImage
        visible: profileUrl !== "" && userName !== "";
        // Size
        height: isMyCard ? 84 : 42;
        width: visible ? height : 0;
        anchors.top: parent.top;
        anchors.topMargin: isMyCard ? 0 : 8;
        anchors.left: parent.left
        clip: true
        Image {
            id: userImage
            source: profileUrl !== "" ? ((0 === profileUrl.indexOf("http")) ? profileUrl : (Account.metaverseServerURL + profileUrl)) : "";
            mipmap: true;
            // Anchors
            anchors.fill: parent
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: Item {
                    width: userImage.width;
                    height: userImage.height;
                    Rectangle {
                        anchors.centerIn: parent;
                        width: userImage.width; // This works because userImage is square
                        height: width;
                        radius: width;
                    }
                }
            }
        }
        AnimatedImage {
            source: "../../icons/profilePicLoading.gif"
            anchors.fill: parent;
            visible: userImage.status != Image.Ready;
        }
        StateImage {
            id: infoHoverImage;
            visible: false;
            imageURL: "../../images/info-icon-2-state.svg";
            size: 32;
            buttonState: 1;
            anchors.centerIn: parent;
        }
        MouseArea {
            anchors.fill: parent
            enabled: (selected && activeTab == "nearbyTab") || isMyCard;
            hoverEnabled: enabled
            onClicked: {
                if (has3DHTML) {
                    userInfoViewer.url = Account.metaverseServerURL + "/users/" + userName;
                    userInfoViewer.visible = true;
                }
            }
            onEntered: infoHoverImage.visible = has3DHTML;
            onExited: infoHoverImage.visible = false;
        }
    }

    // Colored border around avatarImage
    Rectangle {
        id: avatarImageBorder;
        visible: avatarImage.visible;
        anchors.verticalCenter: avatarImage.verticalCenter;
        anchors.horizontalCenter: avatarImage.horizontalCenter;
        width: avatarImage.width + border.width;
        height: avatarImage.height + border.width;
        color: "transparent"
        radius: avatarImage.height;
        border.color: profilePicBorderColor;
        border.width: 4;
    }

    // DisplayName field for my card
    Rectangle {
        id: myDisplayName
        visible: isMyCard
        // Size
        width: parent.width - avatarImage.width - anchors.leftMargin - anchors.rightMargin*2;
        height: 40
        // Anchors
        anchors.top: avatarImage.top
        anchors.topMargin: avatarImage.visible ? 18 : 0;
        anchors.left: avatarImage.right
        anchors.leftMargin: avatarImage.visible ? 5 : 0;
        anchors.rightMargin: 5;
        // Style
        color: hifi.colors.textFieldLightBackground
        border.color: hifi.colors.blueHighlight
        border.width: 0
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                myDisplayNameText.focus = true;
                myDisplayNameText.cursorPosition = myDisplayNameText.positionAt(mouseX - myDisplayNameText.anchors.leftMargin, mouseY, TextInput.CursorOnCharacter);
            }
            onDoubleClicked: {
                myDisplayNameText.selectAll();
                myDisplayNameText.focus = true;
            }
            onEntered: myDisplayName.color = hifi.colors.lightGrayText;
            onExited: myDisplayName.color = hifi.colors.textFieldLightBackground;
        }
        // Edit pencil glyph
        HiFiGlyphs {
            id: editGlyph
            text: hifi.glyphs.editPencil
            // Text Size
            size: displayNameTextPixelSize*1.5
            // Anchors
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            // Style
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: hifi.colors.baseGray
        }
        TextInput {
            id: myDisplayNameText
            // Properties
            text: thisNameCard.displayName
            maximumLength: 256
            clip: true
            // Size
            width: parent.width
            height: parent.height
            // Anchors
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: editGlyph.width + editGlyph.anchors.rightMargin
            // Style
            font.family: "Fira Sans SemiBold"
            font.pixelSize: displayNameTextPixelSize
            selectionColor: hifi.colors.blueAccent
            selectedTextColor: "black"
            // Text Positioning
            verticalAlignment: TextInput.AlignVCenter
            horizontalAlignment: TextInput.AlignLeft
            autoScroll: false;
            // Signals
            onEditingFinished: {
                if (MyAvatar.displayName !== text) {
                    MyAvatar.displayName = text;
                    UserActivityLogger.palAction("display_name_change", text);
                }
                focus = false;
            }
            onFocusChanged: {
                if (focus === true) {
                    myDisplayName.border.width = 1
                    color = "black"
                    autoScroll = true;
                    pal.currentlyEditingDisplayName = true
                } else {
                    myDisplayName.border.width = 0
                    color: hifi.colors.darkGray
                    cursorPosition = 0;
                    autoScroll = false;
                    pal.currentlyEditingDisplayName = false
                }
            }
        }
    }
    // DisplayName container for others' cards
    Item {
        id: displayNameContainer
        visible: !isMyCard && pal.activeTab !== "connectionsTab"
        // Size
        width: parent.width - anchors.leftMargin - avatarImage.width - anchors.leftMargin;
        height: displayNameTextPixelSize + 4
        // Anchors
        anchors.top: avatarImage.top;
        anchors.left: avatarImage.right
        anchors.leftMargin: avatarImage.visible ? 5 : 0;
        // DisplayName Text for others' cards
        FiraSansSemiBold {
            id: displayNameText
            // Properties
            text: thisNameCard.displayName
            elide: Text.ElideRight
            // Size
            width: isAdmin ? Math.min(displayNameTextMetrics.tightBoundingRect.width + 8, parent.width - adminLabelText.width - adminLabelQuestionMark.width + 8) : parent.width
            // Anchors
            anchors.top: parent.top
            anchors.left: parent.left
            // Text Size
            size: displayNameTextPixelSize
            // Text Positioning
            verticalAlignment: Text.AlignTop
            // Style
            color: hifi.colors.darkGray;
            MouseArea {
                anchors.fill: parent
                enabled: selected && pal.activeTab == "nearbyTab" && isPresent;
                hoverEnabled: enabled
                onClicked: {
                    goToUserInDomain(thisNameCard.uuid);
                    UserActivityLogger.palAction("go_to_user_in_domain", thisNameCard.uuid);
                }
                onEntered: {
                    displayNameText.color = hifi.colors.blueHighlight;
                    userNameText.color = hifi.colors.blueHighlight;
                }
                onExited: {
                    displayNameText.color = hifi.colors.darkGray
                    userNameText.color = hifi.colors.blueAccent;
                }
            }
        }
        TextMetrics {
            id:     displayNameTextMetrics
            font:   displayNameText.font
            text:   displayNameText.text
        }
        // "ADMIN" label for other users' cards
        RalewaySemiBold {
            id: adminLabelText
            visible: isAdmin
            text: "ADMIN"
            // Text size
            size: displayNameText.size - 4
            // Anchors
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: displayNameText.right
            // Style
            font.capitalization: Font.AllUppercase
            color: hifi.colors.redHighlight
            // Alignment
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignTop
        }
        // This Rectangle refers to the [?] popup button next to "ADMIN"
        Item {
            id: adminLabelQuestionMark
            visible: isAdmin
            // Size
            width: 20
            height: displayNameText.height
            // Anchors
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: adminLabelText.right
            RalewayRegular {
                id: adminLabelQuestionMarkText
                text: "[?]"
                size: adminLabelText.size
                font.capitalization: Font.AllUppercase
                color: hifi.colors.redHighlight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
            }
            MouseArea {
                anchors.fill: parent
                enabled: isPresent
                hoverEnabled: enabled
                onClicked: letterbox(hifi.glyphs.question,
                "Domain Admin",
                "This user is an admin on this domain. Admins can <b>Silence</b> and <b>Ban</b> other users at their discretion - so be extra nice!")
                onEntered: adminLabelQuestionMarkText.color = "#94132e"
                onExited: adminLabelQuestionMarkText.color = hifi.colors.redHighlight
            }
        }
    }

    // UserName Text
    FiraSansRegular {
        id: userNameText
        // Properties
        text: thisNameCard.userName === "Unknown user" ? "not logged in" : thisNameCard.userName;
        elide: Text.ElideRight
        visible: thisNameCard.userName !== "";
        // Size
        width: parent.width
        height: paintedHeight
        // Anchors
        anchors.top: isMyCard ? myDisplayName.bottom : pal.activeTab == "nearbyTab" ? displayNameContainer.bottom : avatarImage.top //(parent.height - displayNameTextPixelSize/2));
        anchors.bottom: pal.activeTab === "connectionsTab" && !isMyCard ? avatarImage.bottom : undefined
        anchors.left: avatarImage.right;
        anchors.leftMargin: avatarImage.visible ? 5 : 0;
        anchors.rightMargin: 5;
        // Text Size
        size: pal.activeTab == "nearbyTab" || isMyCard ? usernameTextPixelSize : displayNameTextPixelSize;
        // Text Positioning
        verticalAlignment: Text.AlignVCenter;
        // Style
        color: hifi.colors.blueAccent;
        MouseArea {
            anchors.fill: parent
            enabled: selected && pal.activeTab == "nearbyTab" && thisNameCard.userName !== "" && isPresent;
            hoverEnabled: enabled
            onClicked: {
                goToUserInDomain(thisNameCard.uuid);
                UserActivityLogger.palAction("go_to_user_in_domain", thisNameCard.uuid);
            }
            onEntered: {
                displayNameText.color = hifi.colors.blueHighlight;
                userNameText.color = hifi.colors.blueHighlight;
            }
            onExited: {
                displayNameText.color = hifi.colors.darkGray;
                userNameText.color = hifi.colors.blueAccent;
            }
        }
    }
    StateImage {
        id: nameCardConnectionInfoImage
        visible: selected && !isMyCard && pal.activeTab == "connectionsTab" && has3DHTML
        imageURL: "../../images/info-icon-2-state.svg" // PLACEHOLDER!!!
        size: 32;
        buttonState: 0;
        anchors.left: avatarImage.right
        anchors.bottom: parent.bottom
    }
    MouseArea {
        anchors.fill:nameCardConnectionInfoImage
        enabled: selected
        hoverEnabled: true
        onClicked: {
            if (has3DHTML) {
                userInfoViewer.url = Account.metaverseServerURL + "/users/" + userName;
                userInfoViewer.visible = true;
            }
        }
        onEntered: {
            nameCardConnectionInfoImage.buttonState = 1;
        }
        onExited: {
            nameCardConnectionInfoImage.buttonState = 0;
        }
    }
    FiraSansRegular {
        id: nameCardConnectionInfoText
        visible: selected && !isMyCard && pal.activeTab == "connectionsTab" && PlatformInfo.has3DHTML()
        height: displayNameTextPixelSize
        size: displayNameTextPixelSize - 4
        anchors.left: nameCardConnectionInfoImage.right
        anchors.verticalCenter: nameCardConnectionInfoImage.verticalCenter
        anchors.leftMargin: 5
        verticalAlignment: Text.AlignVCenter
        text: "Info"
        color: hifi.colors.baseGray
    }
    HiFiGlyphs {
        id: nameCardRemoveConnectionImage
        visible: selected && !isMyCard && pal.activeTab == "connectionsTab"
        text: hifi.glyphs.close
        size: 24;
        x: 120
        anchors.verticalCenter: nameCardConnectionInfoImage.verticalCenter
        anchors.left: has3DHTML ? nameCardConnectionInfoText.right : avatarImage.right
        anchors.leftMargin: has3DHTML ? 10 : 0
    }
    MouseArea {
        anchors.fill:nameCardRemoveConnectionImage
        enabled: selected
        hoverEnabled: true
        onClicked: {
            // send message to pal.js to forgetConnection
            pal.sendToScript({method: 'removeConnection', params: thisNameCard.userName});
        }
        onEntered: {
            nameCardRemoveConnectionImage.text = hifi.glyphs.closeInverted;
        }
        onExited: {
            nameCardRemoveConnectionImage.text = hifi.glyphs.close;
        }
    }
    FiraSansRegular {
        id: nameCardRemoveConnectionText
        visible: selected && !isMyCard && pal.activeTab == "connectionsTab"
        width: parent.width
        height: displayNameTextPixelSize
        size: displayNameTextPixelSize - 4
        anchors.left: nameCardRemoveConnectionImage.right
        anchors.verticalCenter: nameCardRemoveConnectionImage.verticalCenter
        anchors.leftMargin: 5
        verticalAlignment: Text.AlignVCenter
        text: "Forget"
        color: hifi.colors.baseGray
    }
    HifiControls.Button {
        id: visitConnectionButton
        visible: selected && !isMyCard && pal.activeTab == "connectionsTab"
        text: "Visit"
        enabled: thisNameCard.placeName !== ""
        anchors.verticalCenter: nameCardRemoveConnectionImage.verticalCenter
        x: 240
        onClicked: {
            console.log("Vist user button clicked."); // Remove after debugging.
            AddressManager.goToUser(thisNameCard.userName, false);
            UserActivityLogger.palAction("go_to_user", thisNameCard.userName);
        }
    }

    // VU Meter
    Rectangle {
        id: nameCardVUMeter
        // Size
        width: ((gainSlider.value - gainSlider.minimumValue)/(gainSlider.maximumValue - gainSlider.minimumValue)) * (gainSlider.width);
        height: 8
        // Anchors
        anchors.bottom: isMyCard ? avatarImage.bottom : parent.bottom;
        anchors.bottomMargin: isMyCard ? 0 : height;
        anchors.left: isMyCard ? userNameText.left : parent.left;
        // Style
        radius: 4
        color: "#c5c5c5"
        visible: (!isMyCard && (selected && pal.activeTab == "nearbyTab")) && isPresent
        // Rectangle for the zero-gain point on the VU meter
        Rectangle {
            id: vuMeterZeroGain
            visible: gainSlider.visible
            // Size
            width: 4
            height: 18
            // Style
            color: hifi.colors.darkGray
            // Anchors
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: (-gainSlider.minimumValue)/(gainSlider.maximumValue - gainSlider.minimumValue) * gainSlider.width - 4
        }
        // Rectangle for the VU meter line
        Rectangle {
            id: vuMeterLine
            width: gainSlider.width
            visible: gainSlider.visible
            // Style
            color: vuMeterBase.color
            radius: nameCardVUMeter.radius
            height: nameCardVUMeter.height / 2
            anchors.verticalCenter: nameCardVUMeter.verticalCenter
        }
        // Rectangle for the VU meter base
        Rectangle {
            id: vuMeterBase
            // Anchors
            anchors.fill: parent
            visible: !isMyCard && selected
            // Style
            color: parent.color
            radius: parent.radius
        }
        // Rectangle for the VU meter audio level
        Rectangle {
            id: vuMeterLevel
            visible: !isMyCard && selected
            // Size
            width: (thisNameCard.audioLevel) * parent.width
            // Style
            color: parent.color
            radius: parent.radius
            // Anchors
            anchors.bottom: parent.bottom
            anchors.top: parent.top
            anchors.left: parent.left
        }
        // Gradient for the VU meter audio level
        LinearGradient {
            anchors.fill: vuMeterLevel
            source: vuMeterLevel
            start: Qt.point(0, 0)
            end: Qt.point(parent.width, 0)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2c8e72" }
                GradientStop { position: 0.9; color: "#1fc6a6" }
                GradientStop { position: 0.91; color: "#ea4c5f" }
                GradientStop { position: 1.0; color: "#ea4c5f" }
            }
        }
    }

    // Per-Avatar Gain Slider
    Slider {
        id: gainSlider
        // Size
        width: isMyCard ? thisNameCard.width - 20 : thisNameCard.width;
        height: 14
        // Anchors
        anchors.verticalCenter: nameCardVUMeter.verticalCenter;
        anchors.left: nameCardVUMeter.left;
        // Properties
        visible: (!isMyCard && (selected && pal.activeTab == "nearbyTab")) && isPresent;
        minimumValue: -60.0
        maximumValue: 20.0
        stepSize: 5
        updateValueWhileDragging: true
        value: Users.getAvatarGain(uuid)
        onValueChanged: {
            updateGainFromQML(uuid, value, false);
        }
        onPressedChanged: {
            if (!pressed) {
                updateGainFromQML(uuid, value, true)
            }
        }
        MouseArea {
            anchors.fill: parent
            onWheel: {
                // Do nothing.
            }
            onDoubleClicked: {
                gainSlider.value = 0.0
            }
            onPressed: {
                // Pass through to Slider
                mouse.accepted = false
            }
            onReleased: {
                // the above mouse.accepted seems to make this
                // never get called, nonetheless...
                mouse.accepted = false
            }
        }
        style: SliderStyle {
            groove: Rectangle {
                color: "#c5c5c5"
                implicitWidth: gainSlider.width
                implicitHeight: 4
                radius: 2
                opacity: 0
            }
            handle: Rectangle {
                anchors.centerIn: parent
                color: (control.pressed || control.hovered) ? "#00b4ef" : "#8F8F8F"
                implicitWidth: 10
                implicitHeight: 16
            }
        }
    }

    function updateGainFromQML(avatarUuid, sliderValue, isReleased) {
        if (Users.getAvatarGain(avatarUuid) != sliderValue) {
            Users.setAvatarGain(avatarUuid, sliderValue);
            if (isReleased) {
                UserActivityLogger.palAction("avatar_gain_changed", avatarUuid);
            }
        }
    }

    // Function body by Howard Stearns 2017-01-08
    function goToUserInDomain(avatarUuid) {
        var avatar = AvatarList.getAvatar(avatarUuid);
        if (!avatar || !avatar.position || !avatar.orientation) {
            console.log("This avatar is no longer present. goToUserInDomain() failed.");
            return;
        }
        // This is the last step of what AddressManager.goToUser does, but we don't need to resolve the username.
        MyAvatar.goToLocation(avatar.position, true, Quat.cancelOutRollAndPitch(avatar.orientation), true);
    }
}
