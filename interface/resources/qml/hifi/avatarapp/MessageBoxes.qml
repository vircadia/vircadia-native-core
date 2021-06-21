import QtQuick 2.5

MessageBox {
    id: popup

    function showSpecifyAvatarUrl(url, callback, linkCallback) {
        popup.onButton2Clicked = callback;
        popup.titleText = 'Specify Avatar URL'
        popup.bodyText = 'This will not overwrite your existing favorite if you are wearing one.<br>' +
                '<a href="https://docs.vircadia.com/create/avatars/create-avatars.html">' +
                'Learn to make a custom avatar by opening this link on your desktop.' +
                '</a>'
        popup.inputText.visible = true;
        popup.inputText.placeholderText = 'Enter Avatar Url';
        popup.inputText.text = url;
        popup.inputText.selectAll();
        popup.button1text = 'CANCEL';
        popup.button2text = 'CONFIRM';

        popup.onButton2Clicked = function() {
            if (callback) {
                callback();
            }

            popup.close();
        }

        popup.onLinkClicked = function(link) {
            if (linkCallback) {
                linkCallback(link);
            }
        }

        popup.open();
        popup.inputText.forceActiveFocus();
    }

    function showSpecifyWearableUrl(callback) {
        popup.button2text = 'CONFIRM'
        popup.button1text = 'CANCEL'
        popup.titleText = 'Specify Wearable URL'
        popup.bodyText = 'If you want to add a custom wearable, you can specify the URL of the wearable file here.'

        popup.inputText.visible = true;
        popup.inputText.placeholderText = 'Enter Wearable URL';

        popup.onButton2Clicked = function() {
            if (callback) {
                callback(popup.inputText.text);
            }

            popup.close();
        }

        popup.open();
        popup.inputText.forceActiveFocus();
    }

    function showGetWearables(callback, linkCallback) {
        popup.dialogButtons.yesButton.visible = false;
        popup.button1text = 'CANCEL'
        popup.titleText = 'Get Wearables'
        popup.bodyText = 'Get wearables from <b><a href="app://marketplace">Marketplace.</a></b>' + '<br/>' +
                'Wear wearable from <b><a href="app://purchases">Inventory.</a></b>'

        popup.onLinkClicked = function(link) {
            popup.close();

            if (linkCallback) {
                linkCallback(link);
            }
        }

        popup.open();
    }

    function showDeleteFavorite(favoriteName, callback) {
        popup.titleText = 'Delete Favorite: {AvatarName}'.replace('{AvatarName}', favoriteName)
        popup.bodyText = 'This will delete your favorite. You will retain access to the wearables and avatar that made up the favorite from Inventory.'
        popup.imageSource = null;
        popup.button1text = 'CANCEL'
        popup.button2text = 'DELETE'

        popup.onButton2Clicked = function() {
            popup.close();

            if (callback) {
                callback();
            }
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

            if (callback) {
                callback();
            }
        }
        popup.open();
    }

    function showBuyAvatars(callback, linkCallback) {
        popup.dialogButtons.yesButton.visible = false;
        popup.button1text = 'CANCEL'
        popup.titleText = 'Get Avatars'

        popup.bodyText = 'Get avatars from the Community Bazaar. (Coming soon!)'

        popup.onLinkClicked = function(link) {
            popup.close();

            if (linkCallback) {
                linkCallback(link);
            }
        }

        popup.open();
    }
}

