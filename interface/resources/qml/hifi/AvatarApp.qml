import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQml.Models 2.1
import QtGraphicalEffects 1.0
import controlsUit 1.0 as HifiControls
import stylesUit 1.0
import "avatarapp"

Rectangle {
    id: root
    width: 480
    height: 706

    property bool keyboardEnabled: true
    property bool keyboardRaised: false
    property bool punctuationMode: false

    HifiConstants { id: hifi }

    HifiControls.Keyboard {
        id: keyboard
        z: 1000
        raised: parent.keyboardEnabled && parent.keyboardRaised && HMD.active
        numeric: parent.punctuationMode
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }

    color: style.colors.white
    property string getAvatarsMethod: 'getAvatars'

    signal sendToScript(var message);
    function emitSendToScript(message) {
        sendToScript(message);
    }

    ListModel { // the only purpose of this model is to convert JS object to ListElement
        id: currentAvatarModel
        dynamicRoles: true;
        function makeAvatarEntry(avatarObject) {
            clear();
            append(avatarObject);
            return get(count - 1);
        }
    }

    property var jointNames: []
    property var currentAvatarSettings;
    property bool wearablesFrozen;

    function fetchAvatarModelName(marketId, avatar) {
        var xmlhttp = new XMLHttpRequest();
        var url = "https://highfidelity.com/api/v1/marketplace/items/" + marketId;
        xmlhttp.onreadystatechange = function() {
            if (xmlhttp.readyState === XMLHttpRequest.DONE && xmlhttp.status === 200) {
                try {
                    var marketResponse = JSON.parse(xmlhttp.responseText.trim())

                    if (marketResponse.status === 'success') {
                        avatar.modelName = marketResponse.data.title;
                    }
                }
                catch(err) {
                    //console.error(err);
                }
            }
        }
        xmlhttp.open("GET", url, true);
        xmlhttp.send();
    }

    function getAvatarModelName() {

        if (currentAvatar === null) {
            return '';
        }
        if (currentAvatar.modelName !== undefined) {
            return currentAvatar.modelName;
        } else {
            var marketId = allAvatars.extractMarketId(currentAvatar.avatarUrl);
            if (marketId !== '') {
                fetchAvatarModelName(marketId, currentAvatar);
            }
        }

        var avatarUrl = currentAvatar.avatarUrl;
        var splitted = avatarUrl.split('/');

        return splitted[splitted.length - 1];
    }

    property string avatarName: currentAvatar ? currentAvatar.name : ''
    property string avatarUrl: currentAvatar ? currentAvatar.thumbnailUrl : null
    property bool isAvatarInFavorites: currentAvatar ? allAvatars.findAvatar(currentAvatar.name) !== undefined : false
    property int avatarWearablesCount: currentAvatar ? currentAvatar.wearables.count : 0
    property var currentAvatar: null;
    function setCurrentAvatar(avatar, bookmarkName) {
        var currentAvatarObject = allAvatars.makeAvatarObject(avatar, bookmarkName);
        currentAvatar = currentAvatarModel.makeAvatarEntry(currentAvatarObject);
    }

    property url externalAvatarThumbnailUrl: '../../images/avatarapp/guy-in-circle.svg'

    function fromScript(message) {
        if (message.method === 'initialize') {
            jointNames = message.data.jointNames;
            emitSendToScript({'method' : getAvatarsMethod});
        } else if (message.method === 'wearableUpdated') {
            adjustWearables.refreshWearable(message.entityID, message.wearableIndex, message.properties, message.updateUI);
        } else if (message.method === 'wearablesUpdated') {
            var wearablesModel = currentAvatar.wearables;
            wearablesModel.clear();
            message.wearables.forEach(function(wearable) {
                wearablesModel.append(wearable);
            });
            adjustWearables.refresh(currentAvatar);
        } else if (message.method === 'scaleChanged') {
            currentAvatar.avatarScale = message.value;
            updateCurrentAvatarInBookmarks(currentAvatar);
        } else if (message.method === 'externalAvatarApplied') {
            currentAvatar.avatarUrl = message.avatarURL;
            currentAvatar.thumbnailUrl = allAvatars.makeThumbnailUrl(message.avatarURL);
            currentAvatar.entry.avatarUrl = currentAvatar.avatarUrl;
            currentAvatar.modelName = undefined;
            updateCurrentAvatarInBookmarks(currentAvatar);
        } else if (message.method === 'settingChanged') {
            currentAvatarSettings[message.name] = message.value;
        } else if (message.method === 'changeSettings') {
            currentAvatarSettings = message.settings;
        } else if (message.method === 'bookmarkLoaded') {
            setCurrentAvatar(message.data.currentAvatar, message.data.name);
            var avatarIndex = allAvatars.findAvatarIndex(currentAvatar.name);
            allAvatars.move(avatarIndex, 0, 1);
            view.setPage(0);
        } else if (message.method === 'bookmarkAdded') {
            var avatar = allAvatars.findAvatar(message.bookmarkName);
            if (avatar !== undefined) {
                var avatarObject = allAvatars.makeAvatarObject(message.bookmark, message.bookmarkName);
                for(var prop in avatarObject) {
                    avatar[prop] = avatarObject[prop];
                }
                if (currentAvatar.name === message.bookmarkName) {
                    currentAvatar = currentAvatarModel.makeAvatarEntry(avatarObject);
                }
            } else {
                allAvatars.addAvatarEntry(message.bookmark, message.bookmarkName);
            }
            updateCurrentAvatarInBookmarks(currentAvatar);
        } else if (message.method === 'bookmarkDeleted') {
            pageOfAvatars.isUpdating = true;

            var index = pageOfAvatars.findAvatarIndex(message.name);
            var absoluteIndex = view.currentPage * view.itemsPerPage + index

            allAvatars.remove(absoluteIndex)
            pageOfAvatars.remove(index);

            var itemsOnPage = pageOfAvatars.count;
            var newItemIndex = view.currentPage * view.itemsPerPage + itemsOnPage;

            if (newItemIndex <= (allAvatars.count - 1)) {
                pageOfAvatars.append(allAvatars.get(newItemIndex));
            } else {
                if (!pageOfAvatars.hasGetAvatars()) {
                    pageOfAvatars.appendGetAvatars();
                }
            }

            pageOfAvatars.isUpdating = false;
        } else if (message.method === getAvatarsMethod) {
            var getAvatarsData = message.data;
            allAvatars.populate(getAvatarsData.bookmarks);
            setCurrentAvatar(getAvatarsData.currentAvatar, '');
            displayNameInput.text = getAvatarsData.displayName;
            currentAvatarSettings = getAvatarsData.currentAvatarSettings;

            var bookmarkAvatarIndex = allAvatars.findAvatarIndexByValue(currentAvatar);
            if (bookmarkAvatarIndex === -1) {
                currentAvatar.name = '';
            } else {
                currentAvatar.name = allAvatars.get(bookmarkAvatarIndex).name;
                allAvatars.move(bookmarkAvatarIndex, 0, 1);
            }
            view.setPage(0);
        } else if (message.method === 'updateAvatarInBookmarks') {
            updateCurrentAvatarInBookmarks(currentAvatar);
        } else if (message.method === 'selectAvatarEntity') {
            adjustWearables.selectWearableByID(message.entityID);
        } else if (message.method === 'wearablesFrozenChanged') {
            wearablesFrozen = message.wearablesFrozen;
        }
    }

    function updateCurrentAvatarInBookmarks(avatar) {
        var bookmarkAvatarIndex = allAvatars.findAvatarIndexByValue(avatar);
        if (bookmarkAvatarIndex === -1) {
            avatar.name = '';
            view.setPage(0);
        } else {
            var bookmarkAvatar = allAvatars.get(bookmarkAvatarIndex);
            avatar.name = bookmarkAvatar.name;
            view.selectAvatar(bookmarkAvatar);
        }
    }

    property bool isInManageState: false

    Component.onDestruction: {
        keyboard.raised = false;
    }

    AvatarAppStyle {
        id: style
    }

    AvatarAppHeader {
        id: header
        z: 100

        property string currentPage: "Avatar"
        property bool mainPageVisible: !settings.visible && !adjustWearables.visible

        Binding on currentPage {
            when: settings.visible
            value: "Avatar Settings"
        }
        Binding on currentPage {
            when: adjustWearables.visible
            value: "Adjust Wearables"
        }
        Binding on currentPage {
            when: header.mainPageVisible
            value: "Avatar"
        }

        pageTitle: currentPage
        avatarIconVisible: mainPageVisible
        settingsButtonVisible: mainPageVisible
        onSettingsClicked: {
            displayNameInput.focus = false;
            root.keyboardRaised = false;
            settings.open(currentAvatarSettings, currentAvatar.avatarScale);
        }
    }

    Settings {
        id: settings
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom

        z: 3

        onSaveClicked: function() {
            var avatarSettings = {
                dominantHand : settings.dominantHandIsLeft ? 'left' : 'right',
                hmdAvatarAlignmentType : settings.hmdAvatarAlignmentTypeIsEyes ? 'eyes' : 'head',
                collisionsEnabled : settings.environmentCollisionsOn,
                otherAvatarsCollisionsEnabled : settings.otherAvatarsCollisionsOn,
                animGraphOverrideUrl : settings.avatarAnimationOverrideJSON,
                collisionSoundUrl : settings.avatarCollisionSoundUrl
            };

            emitSendToScript({'method' : 'saveSettings', 'settings' : avatarSettings, 'avatarScale': settings.scaleValue})

            close();
        }
        onCancelClicked: function() {
            emitSendToScript({'method' : 'revertScale', 'avatarScale' : avatarScaleBackup});

            close();
        }

        onScaleChanged: {
            emitSendToScript({'method' : 'setScale', 'avatarScale' : scale})
        }
    }

    AdjustWearables {
        id: adjustWearables
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        jointNames: root.jointNames
        onWearableUpdated: {
            emitSendToScript({'method' : 'adjustWearable', 'entityID' : id, 'wearableIndex' : index, 'properties' : properties})
        }
        onWearableDeleted: {
            emitSendToScript({'method' : 'deleteWearable', 'entityID' : id, 'avatarName' : avatarName});
        }
        onAdjustWearablesOpened: {
            emitSendToScript({'method' : 'adjustWearablesOpened', 'avatarName' : avatarName});
        }
        onAdjustWearablesClosed: {
            emitSendToScript({'method' : 'adjustWearablesClosed', 'save' : status, 'avatarName' : avatarName});
        }
        onWearableSelected: {
            emitSendToScript({'method' : 'selectWearable', 'entityID' : id});
        }
        onAddWearable: {
            emitSendToScript({'method' : 'addWearable', 'avatarName' : avatarName, 'url' : url});
        }

        z: 3
    }

    Rectangle {
        id: mainBlock
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.right: parent.right
        anchors.rightMargin: 30
        anchors.top: header.bottom
        anchors.bottom: favoritesBlock.top

        // TextStyle1
        RalewaySemiBold {
            size: 24;
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 34
        }

        // TextStyle1
        RalewaySemiBold {
            id: displayNameLabel
            size: 24;
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 25
            text: 'Display Name'
        }

        InputField {
            id: displayNameInput

            font.family: "Fira Sans"
            font.pixelSize: 15
            anchors.left: displayNameLabel.right
            anchors.leftMargin: 30
            anchors.verticalCenter: displayNameLabel.verticalCenter
            anchors.right: parent.right
            width: 232

            text: 'ThisIsDisplayName'

            onEditingFinished: {
                emitSendToScript({'method' : 'changeDisplayName', 'displayName' : text})
                focus = false;
            }

            onFocusChanged: {
                root.keyboardRaised = focus;
            }
        }

        ShadowImage {
            id: avatarImage
            width: 134
            height: 134
            anchors.top: displayNameLabel.bottom
            anchors.topMargin: 31
            Binding on source {
                when: avatarUrl !== ''
                value: avatarUrl
            }

            visible: avatarImage.status !== Image.Loading && avatarImage.status !== Image.Error
            fillMode: Image.PreserveAspectCrop
        }

        ShadowImage {
            id: customAvatarImage
            anchors.fill: avatarImage;
            visible: avatarUrl === '' || avatarImage.status === Image.Error
            source: externalAvatarThumbnailUrl
        }

        ShadowRectangle {
            anchors.fill: avatarImage;
            color: 'white'
            visible: avatarImage.status === Image.Loading
            radius: avatarImage.radius

            dropShadowRadius: avatarImage.dropShadowRadius;
            dropShadowHorizontalOffset: avatarImage.dropShadowHorizontalOffset
            dropShadowVerticalOffset: avatarImage.dropShadowVerticalOffset

            Spinner {
                id: spinner
                visible: parent.visible
                anchors.fill: parent;
            }
        }

        AvatarWearablesIndicator {
            anchors.right: avatarImage.right
            anchors.bottom: avatarImage.bottom
            anchors.rightMargin: -radius
            anchors.bottomMargin: 6.08
            wearablesCount: avatarWearablesCount
            visible: avatarWearablesCount !== 0
        }

        RowLayout {
            id: star
            anchors.top: avatarImage.top
            anchors.topMargin: 11
            anchors.left: avatarImage.right
            anchors.leftMargin: 30.5
            anchors.right: parent.right

            spacing: 12.3

            Image {
                width: 21.2
                height: 19.3
                source: isAvatarInFavorites ? '../../images/FavoriteIconActive.svg' : '../../images/FavoriteIconInActive.svg'
                Layout.alignment: Qt.AlignVCenter
            }

            // TextStyle5
            FiraSansSemiBold {
                size: 22;
                Layout.fillWidth: true
                text: isAvatarInFavorites ? avatarName : "Add to Favorites"
                elide: Qt.ElideRight
                Layout.alignment: Qt.AlignVCenter
            }
        }

        MouseArea {
            enabled: !isAvatarInFavorites
            anchors.fill: star
            onClicked: {
                createFavorite.onSaveClicked = function() {
                    var entry = currentAvatar.entry;

                    var wearables = [];
                    for(var i = 0; i < currentAvatar.wearables.count; ++i) {
                        wearables.push(currentAvatar.wearables.get(i));
                    }

                    entry.avatarEntites = wearables;
                    currentAvatar.name = createFavorite.favoriteNameText;

                    emitSendToScript({'method': 'addAvatar', 'name' : currentAvatar.name});
                    createFavorite.close();
                }

                var avatarThumbnail = (avatarUrl === '' || avatarImage.status === Image.Error) ?
                            externalAvatarThumbnailUrl : avatarUrl;

                createFavorite.open(root.currentAvatar.wearables.count, avatarThumbnail);
            }
        }

        // TextStyle3
        RalewayRegular {
            id: avatarNameLabel
            size: 22;
            text: getAvatarModelName();
            elide: Qt.ElideRight

            anchors.right: linkLabel.left
            anchors.left: avatarImage.right
            anchors.leftMargin: 30
            anchors.top: star.bottom
            anchors.topMargin: 11
            property bool hasMarketId: currentAvatar && allAvatars.extractMarketId(currentAvatar.avatarUrl) !== '';

            MouseArea {
                enabled: avatarNameLabel.hasMarketId
                anchors.fill: parent;
                onClicked: emitSendToScript({'method' : 'navigate', 'url' : allAvatars.makeMarketItemUrl(currentAvatar.avatarUrl)})
            }
            color: hasMarketId ? style.colors.blueHighlight : 'black'
        }

        // TextStyle3
        RalewayRegular {
            id: wearablesLabel
            size: 22;
            anchors.left: avatarImage.right
            anchors.leftMargin: 30
            anchors.top: avatarNameLabel.bottom
            anchors.topMargin: 16
            color: 'black'
            text: 'Wearables'
        }

        SquareLabel {
            id: linkLabel
            anchors.right: parent.right
            anchors.verticalCenter: avatarNameLabel.verticalCenter
            glyphText: "."
            glyphSize: 22
            onClicked: {
                popup.showSpecifyAvatarUrl(currentAvatar.avatarUrl, function() {
                    var url = popup.inputText.text;
                    emitSendToScript({'method' : 'applyExternalAvatar', 'avatarURL' : url})
                }, function(link) {
                    Qt.openUrlExternally(link);
                });
            }
        }

        SquareLabel {
            id: adjustLabel
            anchors.right: parent.right
            anchors.verticalCenter: wearablesLabel.verticalCenter
            glyphText: "\ue02e"

            onClicked: {
                if (!AddressManager.isConnected || Entities.canRezAvatarEntities()) {
                    adjustWearables.open(currentAvatar);
                } else {
                    Window.alert("You cannot use wearables on this domain.")
                }

            }
        }

        SquareLabel {
            anchors.right: adjustLabel.left
            anchors.verticalCenter: wearablesLabel.verticalCenter
            anchors.rightMargin: 15
            glyphText: wearablesFrozen ? hifi.glyphs.lock : hifi.glyphs.unlock;

            onClicked: {
                if (!AddressManager.isConnected || Entities.canRezAvatarEntities()) {
                    emitSendToScript({'method' : 'toggleWearablesFrozen'});
                } else {
                    Window.alert("You cannot use wearables on this domain.")
                }
            }
        }
    }

    Rectangle {
        id: favoritesBlock
        height: 407

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        color: style.colors.lightGrayBackground

        // TextStyle1
        RalewaySemiBold {
            id: favoritesLabel
            size: 24;
            anchors.top: parent.top
            anchors.topMargin: 15
            anchors.left: parent.left
            anchors.leftMargin: 30
            text: "Favorites"
        }

        // TextStyle8
        RalewaySemiBold {
            id: manageLabel
            color: style.colors.blueHighlight
            size: 20;
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.right: parent.right
            anchors.rightMargin: 30
            text: isInManageState ? "Back" : "Manage"
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    isInManageState = isInManageState ? false : true;
                }
            }
        }

        Item {
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.right: parent.right
            anchors.rightMargin: 0

            anchors.top: favoritesLabel.bottom
            anchors.topMargin: 20
            anchors.bottom: parent.bottom

            GridView {
                id: view
                anchors.fill: parent
                interactive: false;
                currentIndex: currentAvatarIndexInBookmarksPage();

                function currentAvatarIndexInBookmarksPage() {
                    return (currentAvatar && currentAvatar.name !== '' && !pageOfAvatars.isUpdating) ? pageOfAvatars.findAvatarIndex(currentAvatar.name) : -1;
                }

                property int horizontalSpacing: 18
                property int verticalSpacing: 44
                property int thumbnailWidth: 92
                property int thumbnailHeight: 92

                function selectAvatar(avatar) {
                    emitSendToScript({'method' : 'selectAvatar', 'name' : avatar.name})
                }

                function deleteAvatar(avatar) {
                    emitSendToScript({'method' : 'deleteAvatar', 'name' : avatar.name})
                }

                AvatarsModel {
                    id: allAvatars
                }

                property int itemsPerPage: 8
                property int totalPages: Math.ceil((allAvatars.count + 1) / itemsPerPage)
                property int currentPage: 0;
                onCurrentPageChanged: {
                    currentIndex = Qt.binding(currentAvatarIndexInBookmarksPage);
                }

                property bool hasNext: currentPage < (totalPages - 1)
                property bool hasPrev: currentPage > 0

                function setPage(pageIndex) {
                    pageOfAvatars.isUpdating = true;
                    pageOfAvatars.clear();
                    var start = pageIndex * itemsPerPage;
                    var end = Math.min(start + itemsPerPage, allAvatars.count);

                    for(var itemIndex = 0; start < end; ++start, ++itemIndex) {
                        var avatarItem = allAvatars.get(start)
                        pageOfAvatars.append(avatarItem);
                    }

                    if (pageOfAvatars.count !== itemsPerPage) {
                        pageOfAvatars.appendGetAvatars();
                    }

                    currentPage = pageIndex;
                    pageOfAvatars.isUpdating = false;
                }

                model: AvatarsModel {
                    id: pageOfAvatars

                    property bool isUpdating: false;
                    property var getMoreAvatarsEntry: {'thumbnailUrl' : '', 'name' : '', 'getMoreAvatars' : true}

                    function appendGetAvatars() {
                        append(getMoreAvatarsEntry);
                    }

                    function hasGetAvatars() {
                        return count != 0 && get(count - 1).getMoreAvatars
                    }

                    function removeGetAvatars() {
                        if (hasGetAvatars()) {
                            remove(count - 1)
                        }
                    }
                }

                flow: GridView.FlowLeftToRight

                cellHeight: thumbnailHeight + verticalSpacing
                cellWidth: thumbnailWidth + horizontalSpacing

                delegate: Item {
                    id: delegateRoot
                    height: GridView.view.cellHeight
                    width: GridView.view.cellWidth

                    Item {
                        id: container
                        width: 92
                        height: 92

                        Behavior on y {
                            NumberAnimation {
                                duration: 100
                            }
                        }

                        states: [
                            State {
                                name: "hovered"
                                when: favoriteAvatarMouseArea.containsMouse;
                                PropertyChanges { target: favoriteAvatarMouseArea; anchors.bottomMargin: -5 }
                                PropertyChanges { target: container; y: -5 }
                                PropertyChanges { target: favoriteAvatarImage; dropShadowRadius: 10 }
                                PropertyChanges { target: favoriteAvatarImage; dropShadowVerticalOffset: 6 }
                            },
                            State {
                                name: "getMoreAvatarsHovered"
                                when: getMoreAvatarsMouseArea.containsMouse;
                                PropertyChanges { target: getMoreAvatarsMouseArea; anchors.bottomMargin: -5 }
                                PropertyChanges { target: container; y: -5 }
                                PropertyChanges { target: getMoreAvatarsImage; dropShadowRadius: 10 }
                                PropertyChanges { target: getMoreAvatarsImage; dropShadowVerticalOffset: 6 }
                            }
                        ]

                        property bool highlighted: delegateRoot.GridView.isCurrentItem

                        AvatarThumbnail {
                            id: favoriteAvatarImage
                            externalAvatarThumbnailUrl: root.externalAvatarThumbnailUrl
                            avatarUrl: thumbnailUrl
                            border.color: container.highlighted ? style.colors.blueHighlight : 'transparent'
                            border.width: container.highlighted ? 4 : 0
                            wearablesCount: {
                                return !getMoreAvatars ? wearables.count : 0
                            }

                            visible: !getMoreAvatars

                            MouseArea {
                                id: favoriteAvatarMouseArea
                                anchors.fill: parent
                                anchors.margins: 0
                                enabled: !container.highlighted
                                hoverEnabled: enabled

                                onClicked: {
                                    if (isInManageState) {
                                        var currentItem = delegateRoot.GridView.view.model.get(index);
                                        popup.showDeleteFavorite(currentItem.name, function() {
                                            view.deleteAvatar(currentItem);
                                        });
                                    } else {
                                        if (delegateRoot.GridView.view.currentIndex !== index) {
                                            var currentItem = delegateRoot.GridView.view.model.get(index);
                                            popup.showLoadFavorite(currentItem.name, function() {
                                                view.selectAvatar(currentItem);
                                            });
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.fill: favoriteAvatarImage
                            color: '#AFAFAF'
                            opacity: 0.4
                            radius: 5
                            visible: isInManageState && !container.highlighted && !getMoreAvatars
                        }

                        HiFiGlyphs {
                            anchors.fill: parent
                            text: "{"
                            visible: isInManageState && !container.highlighted && !getMoreAvatars
                            horizontalAlignment: Text.AlignHCenter
                            size: 56
                        }

                        ShadowRectangle {
                            id: getMoreAvatarsImage
                            width: 92
                            height: 92
                            radius: 5
                            color: style.colors.blueHighlight
                            visible: getMoreAvatars && !isInManageState

                            HiFiGlyphs {
                                anchors.centerIn: parent

                                color: 'white'
                                size: 60
                                text: "K"
                            }

                            MouseArea {
                                id: getMoreAvatarsMouseArea
                                anchors.fill: parent
                                hoverEnabled: true

                                onClicked: {
                                    popup.showBuyAvatars(null, function(link) {
                                        emitSendToScript({'method' : 'navigate', 'url' : link})
                                    });
                                }
                            }
                        }
                    }

                    // TextStyle7
                    FiraSansRegular {
                        id: text
                        size: 18;
                        lineHeightMode: Text.FixedHeight
                        lineHeight: 16.9;
                        width: view.thumbnailWidth
                        height: view.verticalSpacing
                        elide: Qt.ElideRight
                        anchors.top: container.bottom
                        anchors.topMargin: 8
                        anchors.horizontalCenter: container.horizontalCenter
                        verticalAlignment: Text.AlignTop
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        text: getMoreAvatars ? 'Get More Avatars' : name
                        visible: !getMoreAvatars || !isInManageState
                    }
                }
            }

        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                width: 40
                height: 40
                color: 'transparent'

                PageIndicator {
                    x: 1
                    hasNext: view.hasNext
                    hasPrev: view.hasPrev
                    onClicked: view.setPage(view.currentPage - 1)
                }
            }

            spacing: 0

            Rectangle {
                width: 40
                height: 40
                color: 'transparent'

                PageIndicator {
                    x: -1
                    isPrevious: false
                    hasNext: view.hasNext
                    hasPrev: view.hasPrev
                    onClicked: view.setPage(view.currentPage + 1)
                }
            }

            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
        }
    }

    MessageBoxes {
        id: popup
    }

    CreateFavoriteDialog {
        avatars: allAvatars
        id: createFavorite
    }
}
