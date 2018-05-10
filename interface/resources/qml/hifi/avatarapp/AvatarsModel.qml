import QtQuick 2.9

ListModel {
    id: model

    function makeThumbnailUrl(avatarUrl) {
        var splittedUrl = avatarUrl.split('/');
        var marketId = splittedUrl[splittedUrl.length - 2];
        var indexOfVSuffix = marketId.indexOf('-v');
        if(indexOfVSuffix !== -1) {
            marketId = marketId.substring(0, indexOfVSuffix);
        }
        var avatarThumbnailUrl = "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/previews/%marketId%/large/hifi-mp-%marketId%.jpg"
            .split('%marketId%').join(marketId);

        return avatarThumbnailUrl;
    }

    function populate(bookmarks) {
        for(var avatarName in bookmarks) {
            var avatarThumbnailUrl = makeThumbnailUrl(bookmarks[avatarName].avatarUrl);

            var avatarEntry = {
                'name' : avatarName,
                'url' : avatarThumbnailUrl,
                'wearables' : bookmarks[avatarName].avatarEntites ? bookmarks[avatarName].avatarEntites : [],
                'entry' : bookmarks[avatarName],
                'getMoreAvatars' : false
            };

            append(avatarEntry);
        }
    }

    function findAvatarIndex(avatarName) {
        for(var i = 0; i < count; ++i) {
            if(get(i).name === avatarName) {
                console.debug('avatar found by index: ', i)
                return i;
            }
        }
        return -1;
    }

    function findAvatar(avatarName) {
        console.debug('AvatarsModel: find avatar by', avatarName);

        var avatarIndex = findAvatarIndex(avatarName);
        if(avatarIndex === -1)
            return undefined;

        return get(avatarIndex);
    }

}
