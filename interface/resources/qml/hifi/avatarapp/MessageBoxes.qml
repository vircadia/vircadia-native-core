import QtQuick 2.5

MessageBox {
    id: popup

    function showSpecifyAvatarUrl(url, callback, linkCallback) {
        popup.onButton2Clicked = callback;
        popup.titleText = 'Specify Avatar URL'
        popup.bodyText = 'This will not overwrite your existing favorite if you are wearing one.<br>' +
                '<a href="https://docs.highfidelity.com/create-and-explore/avatars/create-avatars">' +
                'Learn to make a custom avatar by opening this link on your desktop.' +
                '</a>'
        popup.inputText.visible = true;
        popup.inputText.placeholderText = 'Enter Avatar Url';
        popup.inputText.text = url;
        popup.inputText.selectAll();
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
        popup.inputText.forceActiveFocus();
    }

    property url getWearablesUrl: '../../../images/avatarapp/AvatarIsland.jpg'

    function showGetWearables(callback, linkCallback) {
        popup.button2text = 'AvatarIsland'
        popup.dialogButtons.yesButton.fontCapitalization = Font.MixedCase;
        popup.button1text = 'CANCEL'
        popup.titleText = 'Get Wearables'
        popup.bodyText = 'Buy wearables from <b><a href="app://marketplace">Marketplace.</a></b>' + '<br/>' +
                         'Wear wearables from <b><a href="app://purchases">My Purchases.</a></b>' + '<br/>' + '<br/>' +
                         'Visit “AvatarIsland” to get wearables'

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

    property url getAvatarsUrl: '../../../images/avatarapp/BodyMart.PNG'

    function showBuyAvatars(callback, linkCallback) {
        popup.button2text = 'BodyMart'
        popup.dialogButtons.yesButton.fontCapitalization = Font.MixedCase;
        popup.button1text = 'CANCEL'
        popup.titleText = 'Get Avatars'

        popup.bodyText = 'Buy avatars from <b><a href="app://marketplace">Marketplace.</a></b>' + '<br/>' +
                         'Wear avatars from <b><a href="app://purchases">My Purchases.</a></b>' + '<br/>' + '<br/>' +
                         'Visit “BodyMart” to get free avatars.'

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

