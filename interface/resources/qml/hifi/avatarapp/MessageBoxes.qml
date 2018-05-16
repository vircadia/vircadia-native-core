import QtQuick 2.5

MessageBox {
    id: popup

    function showSpecifyAvatarUrl(callback, linkCallback) {
        popup.onButton2Clicked = callback;
        popup.titleText = 'Specify Avatar URL'
        popup.bodyText = 'If you want to add a custom avatar, you can specify the URL of the avatar file' +
                '(“.fst” extension) here. <a href="https://docs.highfidelity.com/create-and-explore/avatars/create-avatars">Learn to make a custom avatar by opening this link on your desktop.</a>'
        popup.inputText.visible = true;
        popup.inputText.placeholderText = 'Enter Avatar Url';
        popup.inputText.forceActiveFocus();
        popup.button1text = 'CANCEL';
        popup.button2text = 'CONFIRM';

        popup.onButton2Clicked = function() {
            if(callback)
                callback();

            popup.close();
        }

        popup.onLinkClicked = function(link) {
            if(linkCallback)
                linkCallback(link);
        }

        popup.open();
    }

    property url getWearablesUrl: '../../../images/avatarapp/hifi-place-get-wearables.png'

    function showGetWearables(callback, linkCallback) {
        popup.button2text = 'AvatarIsland'
        popup.button1text = 'CANCEL'
        popup.titleText = 'Get Wearables'
        popup.bodyText = 'Buy wearables from <a href="app://marketplace">Marketplace</a>' + '\n' +
                         'Wear wearable from <a href="app://purchases">My Purchases</a>' + '\n' +
                         'You can visit the domain “AvatarIsland”' + '\n' +
                         'to get wearables'

        popup.imageSource = getWearablesUrl;
        popup.onButton2Clicked = function() {
            popup.close();

            if(callback)
                callback();
        }

        popup.onLinkClicked = function(link) {
            popup.close();

            if(linkCallback)
                linkCallback(link);
        }

        popup.open();
    }

    function showDeleteFavorite(favoriteName, callback) {
        popup.titleText = 'Delete Favorite: {AvatarName}'.replace('{AvatarName}', favoriteName)
        popup.bodyText = 'This will delete your favorite. You will retain access to the wearables and avatar that made up the favorite from My Purchases.'
        popup.imageSource = null;
        popup.button1text = 'CANCEL'
        popup.button2text = 'DELETE'

        popup.onButton2Clicked = function() {
            popup.close();

            if(callback)
                callback();
        }
        popup.open();
    }

    function showLoadFavorite(favoriteName, callback) {
        popup.button2text = 'CONFIRM'
        popup.button1text = 'CANCEL'
        popup.titleText = 'Load Favorite: {AvatarName}'.replace('{AvatarName}', favoriteName)
        popup.bodyText = 'This will switch your current avatar and wearables that you are wearing with a new avatar and wearables.'
        popup.imageSource = null;
        popup.onButton2Clicked = function() {
            popup.close();

            if(callback)
                callback();
        }
        popup.open();
    }

    property url getAvatarsUrl: '../../../images/avatarapp/hifi-place-get-avatars.png'

    function showBuyAvatars(callback, linkCallback) {
        popup.button2text = 'BodyMart'
        popup.button1text = 'CANCEL'
        popup.titleText = 'Get Avatars'

        popup.bodyText = 'Buy avatars from <a href="app://marketplace">Marketplace</a>' + '\n' +
                         'Wear avatars from <a href="app://purchases">My Purchases</a>' + '\n' +
                         'You can visit the domain “BodyMart”' + '\n' +
                         'to get avatars'

        popup.imageSource = getAvatarsUrl;
        popup.onButton2Clicked = function() {
            popup.close();

            if(callback)
                callback();
        }

        popup.onLinkClicked = function(link) {
            popup.close();

            if(linkCallback)
                linkCallback(link);
        }

        popup.open();
    }
}

