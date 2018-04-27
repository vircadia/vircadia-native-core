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

    property string selectedAvatarId: ''
    onSelectedAvatarIdChanged: {
        console.debug('selectedAvatarId: ', selectedAvatarId)
    }

    property var selectedAvatar: selectedAvatarId !== '' ? allAvatars.findAvatar(selectedAvatarId) : undefined
    onSelectedAvatarChanged: {
        console.debug('selectedAvatar: ', selectedAvatar ? selectedAvatar.url : selectedAvatar)
    }

    function isEqualById(avatar, avatarId) {
        return (avatar.url + avatar.name) === avatarId
    }

    property string avatarName: selectedAvatar ? selectedAvatar.name : ''
    property string avatarUrl: selectedAvatar ? selectedAvatar.url : null
    property int avatarWearablesCount: selectedAvatar && selectedAvatar.wearables !== '' ? selectedAvatar.wearables.split('|').length : 0
    property bool isAvatarInFavorites: selectedAvatar ? selectedAvatar.favorite : false

    property bool isInManageState: false

    Component.onCompleted: {
        for(var i = 0; i < allAvatars.count; ++i) {
            var originalUrl = allAvatars.get(i).url;
            if(originalUrl !== '') {
                var resolvedUrl = Qt.resolvedUrl(originalUrl);
                console.debug('url: ', originalUrl, 'resolved: ', resolvedUrl);
                allAvatars.setProperty(i, 'url', resolvedUrl);
            }
        }

        view.selectAvatar(allAvatars.get(1));
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
                    selectedAvatar.favorite = true;
                    pageOfAvatars.setProperty(view.currentIndex, 'favorite', selectedAvatar.favorite)
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
                    adjustWearables.open();
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
                                'url': '../../images/samples/hifi-mp-e76946cc-c272-4adf-9bb6-02cde0a4b57d-2.png',
                                'name': 'Lexi' + (++debug_newAvatarIndex),
                                'wearables': '',
                                'favorite': false
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
                    selectedAvatarId = avatar.url + avatar.name;
                    var avatarIndex = allAvatars.findAvatarIndex(selectedAvatarId);
                    allAvatars.move(avatarIndex, 0, 1);
                    view.setPage(0);
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
                    property var getMoreAvatars: {'url' : '', 'name' : 'Get More Avatars'}

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
                        append(getMoreAvatars);
                    }

                    function hasGetAvatars() {
                        return count != 0 && get(count - 1).url === ''
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
                            wearablesCount: (wearables && wearables !== '') ? wearables.split('|').length : 0
                            onWearablesCountChanged: {
                                console.debug('delegate: AvatarThumbnail.wearablesCount: ', wearablesCount)
                            }

                            visible: url !== ''

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
                            visible: url === '' && !isInManageState

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
                        text: name
                        visible: url !== '' || !isInManageState
                    }
                }
            }

        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter

            HiFiGlyphs {
                rotation: 180
                text: "\ue01d";
                size: 50
                color: view.hasPrev ? 'black' : 'gray'
                visible: view.hasNext || view.hasPrev
                horizontalAlignment: Text.AlignHCenter
                MouseArea {
                    anchors.fill: parent
                    enabled: view.hasPrev
                    onClicked: {
                        view.setPage(view.currentPage - 1)
                    }
                }
            }

            spacing: 0

            HiFiGlyphs {
                text: "\ue01d";
                size: 50
                color: view.hasNext ? 'black' : 'gray'
                visible: view.hasNext || view.hasPrev
                horizontalAlignment: Text.AlignHCenter
                MouseArea {
                    anchors.fill: parent
                    enabled: view.hasNext
                    onClicked: {
                        view.setPage(view.currentPage + 1)
                    }
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
        onVisibleChanged: {
            if(visible) {
                console.debug('selectedAvatar.wearables: ', selectedAvatar.wearables)
                selectedAvatar.wearables = 'hat|sunglasses|bracelet'
                pageOfAvatars.setProperty(view.currentIndex, 'wearables', selectedAvatar.wearables)
            }
        }

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

                        var avatar = {
                            'url': '../../images/samples/hifi-mp-e76946cc-c272-4adf-9bb6-02cde0a4b57d-2.png',
                            'name': 'Lexi' + (++newAvatarIndex),
                            'wearables': '',
                            'favorite': false
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
