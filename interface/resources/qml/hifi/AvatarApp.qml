import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQml.Models 2.1
import QtGraphicalEffects 1.0
import "../controls-uit" as HifiControls
import "../styles-uit"
import "avatarapp"

Rectangle {
    id: root
    width: 480
	height: 706
    color: style.colors.white
    property string getAvatarsMethod: 'getAvatars'

    signal sendToScript(var message);
    function emitSendToScript(message) {
        console.debug('AvatarApp.qml: emitting sendToScript: ', JSON.stringify(message, null, '\t'));
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

    property var jointNames;

    property var currentAvatar: null;
    function setCurrentAvatar(avatar) {
        var currentAvatarObject =  {
            'name' : '',
            'url' : allAvatars.makeThumbnailUrl(avatar.avatarUrl),
            'wearables' : avatar.avatarEntites ? avatar.avatarEntites : [],
            'entry' : avatar,
            'getMoreAvatars' : false
        };

        currentAvatar = currentAvatarModel.makeAvatarEntry(currentAvatarObject);
        console.debug('AvatarApp.qml: currentAvatarObject: ', currentAvatarObject, 'currentAvatar: ', currentAvatar, JSON.stringify(currentAvatar.wearables, 0, 4));
        console.debug('currentAvatar.wearables: ', currentAvatar.wearables);
    }

    function fromScript(message) {
        console.debug('AvatarApp.qml: fromScript: ', JSON.stringify(message, null, '\t'))

        if(message.method === 'initialize') {
            jointNames = message.reply.jointNames;
            emitSendToScript({'method' : getAvatarsMethod});
        } else if(message.method === 'wearableUpdated') {
            adjustWearables.refreshWearable(message.entityID, message.wearableIndex, message.properties);
        } else if(message.method === 'wearablesUpdated') {
            var wearablesModel = currentAvatarModel.get(0).wearables;
            wearablesModel.clear();
            message.wearables.forEach(function(wearable) {
                wearablesModel.append(wearable);
            });
            console.debug('wearablesUpdated: ', JSON.stringify(wearablesModel, 0, 4), '*****', JSON.stringify(message.wearables, 0, 4));
            adjustWearables.refresh(root.currentAvatar);
        } else if(message.method === 'bookmarkLoaded') {
            setCurrentAvatar(message.reply.currentAvatar);
            selectedAvatarId = message.reply.name;
            var avatarIndex = allAvatars.findAvatarIndex(selectedAvatarId);
            allAvatars.move(avatarIndex, 0, 1);
            view.setPage(0);
        } else if(message.method === getAvatarsMethod) {
            var getAvatarsReply = message.reply;
            allAvatars.populate(getAvatarsReply.bookmarks);

            var currentAvatar = getAvatarsReply.currentAvatar;
            console.debug('currentAvatar: ', JSON.stringify(currentAvatar, null, '\t'));
            var selectedAvatarIndex = -1;

            // 2DO: find better way of determining selected avatar in bookmarks
            console.debug('allAvatars.count: ', allAvatars.count);
            for(var i = 0; i < allAvatars.count; ++i) {
                var thesame = true;
                for(var prop in currentAvatar) {
                    // console.debug('prop', prop);

                    var v1 = currentAvatar[prop];
                    var v2 = allAvatars.get(i).entry[prop];
                    console.debug('v1', v1, 'v2', v2);

                    var s1 = JSON.stringify(v1);
                    var s2 = JSON.stringify(v2);

                    // console.debug('comparing\n', s1, 'to\n', s2, '...');
                    if(s1 !== s2) {
                        if(!(Array.isArray(v1) && v1.length === 0 && v2 === undefined)) {
                            thesame = false;
                            break;
                        }
                    }
                    // console.debug('values seems to be the same...');
                }
                if(thesame) {
                    selectedAvatarIndex = i;
                    break;
                }
            }

            console.debug('selectedAvatarIndex = -1, avatar is not favorite')

            setCurrentAvatar(currentAvatar);

            if(selectedAvatarIndex === -1) {
                selectedAvatar = root.currentAvatar;
                view.setPage(0);

                console.debug('selectedAvatar = ', JSON.stringify(selectedAvatar, null, '\t'))
            } else {
                view.selectAvatar(allAvatars.get(selectedAvatarIndex));
            }
        } else if(message.method === 'selectAvatarEntity') {
            adjustWearables.selectWearableByID(message.entityID);
        }
    }

    property string selectedAvatarId: ''
    onSelectedAvatarIdChanged: {
        console.debug('selectedAvatarId: ', selectedAvatarId)
        selectedAvatar = allAvatars.findAvatar(selectedAvatarId);
    }

    property var selectedAvatar;
    onSelectedAvatarChanged: {
        console.debug('onSelectedAvatarChanged.selectedAvatar: ', JSON.stringify(selectedAvatar, null, '\t'));
    }

    function isEqualById(avatar, avatarId) {
        return (avatar.name) === avatarId
    }

    property string avatarName: selectedAvatar ? selectedAvatar.name : ''
    property string avatarUrl: selectedAvatar ? selectedAvatar.url : null
    property int avatarWearablesCount: selectedAvatar ? selectedAvatar.wearables.count : 0
    property bool isAvatarInFavorites: selectedAvatar ? allAvatars.findAvatar(selectedAvatar.name) !== undefined : false

    property bool isInManageState: false

    Component.onCompleted: {
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
            settings.open();
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
            close();
        }
        onCancelClicked: function() {
            close();
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
            emitSendToScript({'method' : 'adjustWearablesOpened'});
        }
        onAdjustWearablesClosed: {
            emitSendToScript({'method' : 'adjustWearablesClosed'});
        }
        onWearableSelected: {
            emitSendToScript({'method' : 'selectWearable', 'entityID' : id});
        }

        z: 3
    }

    Rectangle {
        id: mainBlock
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: favoritesBlock.top

        TextStyle1 {
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.top: parent.top
            anchors.topMargin: 34
        }

        TextStyle1 {
            id: displayNameLabel
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.top: parent.top
            anchors.topMargin: 34
            text: 'Display Name'
        }

        TextField {
            id: displayNameInput
            anchors.left: displayNameLabel.right
            anchors.leftMargin: 30
            anchors.verticalCenter: displayNameLabel.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 36
            width: 232

            property bool error: text === '';
            text: 'ThisIsDisplayName'

            states: [
                State {
                    name: "hovered"
                    when: displayNameInput.hovered && !displayNameInput.focus && !displayNameInput.error;
                    PropertyChanges { target: displayNameInputBackground; color: '#afafaf' }
                },
                State {
                    name: "focused"
                    when: displayNameInput.focus && !displayNameInput.error
                    PropertyChanges { target: displayNameInputBackground; color: '#f2f2f2' }
                    PropertyChanges { target: displayNameInputBackground; border.color: '#00b4ef' }
                },
                State {
                    name: "error"
                    when: displayNameInput.error
                    PropertyChanges { target: displayNameInputBackground; color: '#f2f2f2' }
                    PropertyChanges { target: displayNameInputBackground; border.color: '#e84e62' }
                }
            ]

            background: Rectangle {
                id: displayNameInputBackground
                implicitWidth: 200
                implicitHeight: 40
                color: '#d4d4d4'
                border.color: '#afafaf'
                border.width: 1
                radius: 2
            }

            HiFiGlyphs {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                size: 36
                text: "\ue00d"
            }
        }

        ShadowImage {
            id: avatarImage
            width: 134
            height: 134
            anchors.left: displayNameLabel.left
            anchors.top: displayNameLabel.bottom
            anchors.topMargin: 31
            source: avatarUrl
        }

        AvatarWearablesIndicator {
            anchors.right: avatarImage.right
            anchors.bottom: avatarImage.bottom
            anchors.rightMargin: -radius
            anchors.bottomMargin: 6.08
            wearablesCount: avatarWearablesCount
            visible: avatarWearablesCount !== 0
        }

        Row {
            id: star
            anchors.top: parent.top
            anchors.topMargin: 119
            anchors.left: avatarImage.right
            anchors.leftMargin: 30.5

            spacing: 12.3

            Image {
                width: 21.2
                height: 19.3
                source: isAvatarInFavorites ? '../../images/FavoriteIconActive.svg' : '../../images/FavoriteIconInActive.svg'
                anchors.verticalCenter: parent.verticalCenter
            }

            TextStyle5 {
                text: isAvatarInFavorites ? avatarName : "Add to Favorites"
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            enabled: !isAvatarInFavorites
            anchors.fill: star
            onClicked: {
                console.debug('selectedAvatar.url', selectedAvatar.url)
                createFavorite.onSaveClicked = function() {
                    var newAvatar = JSON.parse(JSON.stringify(selectedAvatar));
                    newAvatar.name = createFavorite.favoriteNameText;
                    allAvatars.append(newAvatar);
                    view.selectAvatar(newAvatar);
                    createFavorite.close();
                }

                createFavorite.open(selectedAvatar);
            }
        }

        TextStyle3 {
            id: avatarNameLabel
            text: avatarName
            anchors.left: avatarImage.right
            anchors.leftMargin: 30
            anchors.top: parent.top
            anchors.topMargin: 154
        }

        TextStyle3 {
            id: wearablesLabel
            anchors.left: avatarImage.right
            anchors.leftMargin: 30
            anchors.top: avatarNameLabel.bottom
            anchors.topMargin: 16
            text: 'Wearables'
        }

        SquareLabel {
            anchors.right: parent.right
            anchors.rightMargin: 30
            anchors.verticalCenter: avatarNameLabel.verticalCenter
            glyphText: "."
            glyphSize: 22

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    popup.titleText = 'Specify Avatar URL'
                    popup.bodyText = 'If you want to add a custom avatar, you can specify the URL of the avatar file' +
                            '(“.fst” extension) here. <a href="#">Learn to make a custom avatar by opening this link on your desktop.</a>'
                    popup.inputText.visible = true;
                    popup.inputText.placeholderText = 'Enter Avatar Url';
                    popup.button1text = 'CANCEL';
                    popup.button2text = 'CONFIRM';
                    popup.onButton2Clicked = function() {
                        popup.close();
                    }

                    popup.open();
                }
            }
        }

        SquareLabel {
            anchors.right: parent.right
            anchors.rightMargin: 30
            anchors.verticalCenter: wearablesLabel.verticalCenter
            glyphText: "\ue02e"

            visible: avatarWearablesCount !== 0

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.debug('adjustWearables.open');
                    adjustWearables.open(currentAvatar);
                }
            }
        }

        TextStyle3 {
            anchors.right: parent.right
            anchors.rightMargin: 30
            anchors.verticalCenter: wearablesLabel.verticalCenter
            font.underline: true
            text: "Add"
            visible: avatarWearablesCount === 0

            MouseArea {
                anchors.fill: parent
                property url getWearablesUrl: '../../images/samples/hifi-place-77312e4b-6f48-4eb4-87e2-50444d8e56d1.png'

                // debug only
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                property int debug_newAvatarIndex: 0

                onClicked: {
                    if(mouse.button == Qt.RightButton) {

                        for(var i = 0; i < 3; ++i)
                        {
                            console.debug('adding avatar...');

                            var avatar = {
                                'url': Qt.resolvedUrl('../../images/samples/hifi-mp-e76946cc-c272-4adf-9bb6-02cde0a4b57d-2.png'),
                                'name': 'Lexi' + (++debug_newAvatarIndex),
                                'wearables': []
                            };

                            allAvatars.append(avatar)

                            if(pageOfAvatars.hasGetAvatars())
                                pageOfAvatars.removeGetAvatars();

                            if(pageOfAvatars.count !== view.itemsPerPage)
                                pageOfAvatars.append(avatar);

                            if(pageOfAvatars.count !== view.itemsPerPage)
                                pageOfAvatars.appendGetAvatars();
                        }

                        return;
                    }

                    popup.button2text = 'AvatarIsland'
                    popup.button1text = 'CANCEL'
                    popup.titleText = 'Get Wearables'
                    popup.bodyText = 'Buy wearables from <a href="https://fake.link">Marketplace</a>' + '\n' +
                                     'Wear wearable from <a href="https://fake.link">My Purchases</a>' + '\n' +
                                     'You can visit the domain “AvatarIsland”' + '\n' +
                                     'to get wearables'

                    popup.imageSource = getWearablesUrl;
                    popup.onButton2Clicked = function() {
                        popup.close();
                        gotoAvatarAppPanel.visible = true;
                    }
                    popup.open();
                }
            }
        }
    }

    Rectangle {
        id: favoritesBlock
        height: 369

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        color: style.colors.lightGrayBackground

        TextStyle1 {
            id: favoritesLabel
            anchors.top: parent.top
            anchors.topMargin: 9
            anchors.left: parent.left
            anchors.leftMargin: 30
            text: "Favorites"
        }

        TextStyle8 {
            id: manageLabel
            anchors.top: parent.top
            anchors.topMargin: 9
            anchors.right: parent.right
            anchors.rightMargin: 30
            text: isInManageState ? "Back" : "Manage"
            color: style.colors.blueHighlight
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
            anchors.topMargin: 9
            anchors.bottom: parent.bottom

            GridView {
                id: view
                anchors.fill: parent
                interactive: false;
                currentIndex: (selectedAvatarId !== '' && !pageOfAvatars.isUpdating) ? pageOfAvatars.findAvatar(selectedAvatarId) : -1

                property int horizontalSpacing: 18
                property int verticalSpacing: 36

                function selectAvatar(avatar) {
                    emitSendToScript({'method' : 'selectAvatar', 'name' : avatar.name})
                }

                AvatarsModel {
                    id: allAvatars

                    function findAvatarIndex(avatarId) {
                        for(var i = 0; i < count; ++i) {
                            if(isEqualById(get(i), avatarId)) {
                                console.debug('avatar found by index: ', i)
                                return i;
                            }
                        }
                        return -1;
                    }

                    function findAvatar(avatarId) {
                        console.debug('AvatarsModel: find avatar by', avatarId);

                        var avatarIndex = findAvatarIndex(avatarId);
                        if(avatarIndex === -1)
                            return undefined;

                        return get(avatarIndex);
                    }
                }

                property int itemsPerPage: 8
                property int totalPages: Math.ceil((allAvatars.count + 1) / itemsPerPage)
                onTotalPagesChanged: {
                    console.debug('total pages: ', totalPages)
                }

                property int currentPage: 0;
                onCurrentPageChanged: {
                    console.debug('currentPage: ', currentPage)
                    currentIndex = Qt.binding(function() {
                        return (selectedAvatarId !== '' && !pageOfAvatars.isUpdating) ? pageOfAvatars.findAvatar(selectedAvatarId) : -1
                    })
                }

                property bool hasNext: currentPage < (totalPages - 1)
                onHasNextChanged: {
                    console.debug('hasNext: ', hasNext)
                }

                property bool hasPrev: currentPage > 0
                onHasPrevChanged: {
                    console.debug('hasPrev: ', hasPrev)
                }

                function setPage(pageIndex) {
                    pageOfAvatars.isUpdating = true;
                    pageOfAvatars.clear();
                    var start = pageIndex * itemsPerPage;
                    var end = Math.min(start + itemsPerPage, allAvatars.count);

                    for(var itemIndex = 0; start < end; ++start, ++itemIndex) {
                        var avatarItem = allAvatars.get(start)
                        console.debug('getting ', start, avatarItem)
                        pageOfAvatars.append(avatarItem);
                    }

                    if(pageOfAvatars.count !== itemsPerPage)
                        pageOfAvatars.appendGetAvatars();

                    currentPage = pageIndex;
                    console.debug('switched to the page with', pageOfAvatars.count, 'items')
                    pageOfAvatars.isUpdating = false;
                }

                model: ListModel {
                    id: pageOfAvatars

                    property bool isUpdating: false;
                    property var getMoreAvatarsEntry: {'url' : '', 'name' : '', 'getMoreAvatars' : true}

                    function findAvatar(avatarId) {
                        console.debug('pageOfAvatars.findAvatar: ', avatarId);

                        for(var i = 0; i < count; ++i) {
                            if(isEqualById(get(i), avatarId)) {
                                console.debug('avatar found by index: ', i)
                                return i;
                            }
                        }

                        return -1;
                    }

                    function appendGetAvatars() {
                        append(getMoreAvatarsEntry);
                    }

                    function hasGetAvatars() {
                        return count != 0 && get(count - 1).getMoreAvatars
                    }

                    function removeGetAvatars() {
                        if(hasGetAvatars()) {
                            remove(count - 1)
                            console.debug('removed get avatars...');
                        }
                    }
                }

                flow: GridView.FlowLeftToRight

                cellHeight: 92 + verticalSpacing
                cellWidth: 92 + horizontalSpacing

                delegate: Item {
                    id: delegateRoot
                    height: GridView.view.cellHeight
                    width: GridView.view.cellWidth

                    Item {
                        id: container
                        width: 92
                        height: 92

                        states: [
                            State {
                                name: "hovered"
                                when: favoriteAvatarMouseArea.containsMouse;
                                PropertyChanges { target: container; y: -5 }
                                PropertyChanges { target: favoriteAvatarImage; dropShadowRadius: 10 }
                                PropertyChanges { target: favoriteAvatarImage; dropShadowVerticalOffset: 6 }
                            }
                        ]

                        property bool highlighted: delegateRoot.GridView.isCurrentItem

                        AvatarThumbnail {
                            id: favoriteAvatarImage
                            imageUrl: url
                            border.color: container.highlighted ? style.colors.blueHighlight : 'transparent'
                            border.width: container.highlighted ? 2 : 0
                            wearablesCount: {
                                console.debug('getMoreAvatars: ', getMoreAvatars, 'name: ', name);
                                return !getMoreAvatars ? wearables.count : 0
                            }
                            onWearablesCountChanged: {
                                console.debug('delegate: AvatarThumbnail.wearablesCount: ', wearablesCount)
                            }

                            visible: !getMoreAvatars

                            MouseArea {
                                id: favoriteAvatarMouseArea
                                anchors.fill: parent
                                enabled: !container.highlighted
                                hoverEnabled: enabled

                                property url getWearablesUrl: '../../images/samples/hifi-place-77312e4b-6f48-4eb4-87e2-50444d8e56d1.png'

                                onClicked: {
                                    if(isInManageState) {
                                        var currentItem = delegateRoot.GridView.view.model.get(index);

                                        popup.titleText = 'Delete Favorite: {AvatarName}'.replace('{AvatarName}', currentItem.name)
                                        popup.bodyText = 'This will delete your favorite. You will retain access to the wearables and avatar that made up the favorite from My Purchases.'
                                        popup.imageSource = null;
                                        popup.button1text = 'CANCEL'
                                        popup.button2text = 'DELETE'

                                        popup.onButton2Clicked = function() {
                                            popup.close();

                                            pageOfAvatars.isUpdating = true;

                                            console.debug('removing ', index)

                                            var absoluteIndex = view.currentPage * view.itemsPerPage + index
                                            console.debug('removed ', absoluteIndex, 'view.currentPage', view.currentPage,
                                                          'view.itemsPerPage: ', view.itemsPerPage, 'index', index, 'pageOfAvatars', pageOfAvatars, 'pageOfAvatars.count', pageOfAvatars)

                                            allAvatars.remove(absoluteIndex)
                                            pageOfAvatars.remove(index);

                                            var itemsOnPage = pageOfAvatars.count;
                                            var newItemIndex = view.currentPage * view.itemsPerPage + itemsOnPage;

                                            console.debug('newItemIndex: ', newItemIndex, 'allAvatars.count - 1: ', allAvatars.count - 1, 'pageOfAvatars.count:', pageOfAvatars.count);

                                            if(newItemIndex <= (allAvatars.count - 1)) {
                                                pageOfAvatars.append(allAvatars.get(newItemIndex));
                                            } else {
                                                if(!pageOfAvatars.hasGetAvatars())
                                                    pageOfAvatars.appendGetAvatars();
                                            }

                                            console.debug('removed ', absoluteIndex, 'newItemIndex: ', newItemIndex, 'allAvatars.count:', allAvatars.count, 'pageOfAvatars.count:', pageOfAvatars.count)
                                            pageOfAvatars.isUpdating = false;
                                        };

                                        popup.open();

                                    } else {
                                        if(delegateRoot.GridView.view.currentIndex !== index) {
                                            var currentItem = delegateRoot.GridView.view.model.get(index);

                                            popup.button2text = 'CONFIRM'
                                            popup.button1text = 'CANCEL'
                                            popup.titleText = 'Load Favorite: {AvatarName}'.replace('{AvatarName}', currentItem.name)
                                            popup.bodyText = 'This will switch your current avatar and wearables that you are wearing with a new avatar and wearables.'
                                            popup.imageSource = null;
                                            popup.onButton2Clicked = function() {
                                                popup.close();
                                                view.selectAvatar(currentItem);
                                            }
                                            popup.open();
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
                            visible: isInManageState && !container.highlighted && url !== ''
                        }

                        HiFiGlyphs {
                            anchors.fill: parent
                            text: "{"
                            visible: isInManageState && !container.highlighted && url !== ''
                            horizontalAlignment: Text.AlignHCenter
                            size: 56
                        }

                        ShadowRectangle {
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
                                anchors.fill: parent
                                property url getAvatarsUrl: '../../images/samples/hifi-place-get-avatars.png'

                                onClicked: {
                                    console.debug('getAvatarsUrl: ', getAvatarsUrl);

                                    popup.button2text = 'BodyMart'
                                    popup.button1text = 'CANCEL'
                                    popup.titleText = 'Get Avatars'

                                    popup.bodyText = 'Buy avatars from <a href="https://fake.link">Marketplace</a>' + '\n' +
                                                     'Wear avatars from <a href="https://fake.link">My Purchases</a>' + '\n' +
                                                     'You can visit the domain “BodyMart”' + '\n' +
                                                     'to get avatars'

                                    popup.imageSource = getAvatarsUrl;
                                    popup.onButton2Clicked = function() {
                                        popup.close();
                                        gotoAvatarAppPanel.visible = true;
                                    }
                                    popup.open();
                                }
                            }
                        }
                    }

                    TextStyle7 {
                        id: text
                        width: 92
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
            anchors.bottomMargin: 19
        }
    }

    MessageBox {
        id: popup
    }

    CreateFavoriteDialog {
        id: createFavorite
    }

    Rectangle {
        id: gotoAvatarAppPanel
        anchors.fill: parent
        anchors.leftMargin: 19
        anchors.rightMargin: 19

        // color: 'green'
        visible: false

        Rectangle {
            width: 442
            height: 447
            // color: 'yellow'

            anchors.bottom: parent.bottom
            anchors.bottomMargin: 259

            TextStyle1 {
                anchors.fill: parent
                horizontalAlignment: "AlignHCenter"
                wrapMode: "WordWrap"
                text: "You are teleported to “AvatarIsland” VR world and you buy a hat, sunglasses and a bracelet."
            }
        }

        Rectangle {
            width: 442
            height: 177
            // color: 'yellow'

            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40

            TextStyle1 {
                anchors.fill: parent
                horizontalAlignment: "AlignHCenter"
                wrapMode: "WordWrap"
                text: '<a href="https://fake.link">Click here to open the Avatar app.</a>'

                MouseArea {
                    anchors.fill: parent
                    property int newAvatarIndex: 0

                    onClicked: {
                        gotoAvatarAppPanel.visible = false;

                        var i = allAvatars.count + 1;
                        var url = allAvatars.urls[i++ % allAvatars.urls.length]

                        var avatar = {
                            'url': Qt.resolvedUrl(url),
                            'name': 'Lexi' + (++newAvatarIndex),
                            'wearables': []
                        };

                        allAvatars.append(avatar)

                        if(pageOfAvatars.hasGetAvatars())
                            pageOfAvatars.removeGetAvatars();

                        if(pageOfAvatars.count !== view.itemsPerPage)
                            pageOfAvatars.append(avatar);

                        if(pageOfAvatars.count !== view.itemsPerPage)
                            pageOfAvatars.appendGetAvatars();

                        console.debug('avatar appended: allAvatars.count: ', allAvatars.count, 'pageOfAvatars.count: ', pageOfAvatars.count);
                    }
                }
            }
        }
    }
}
