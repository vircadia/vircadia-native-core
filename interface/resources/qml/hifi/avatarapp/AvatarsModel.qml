import QtQuick 2.9

ListModel {
    id: model

    function populate(bookmarks) {
        for(var avatarName in bookmarks) {
            var splittedUrl = bookmarks[avatarName].avatarUrl.split('/');
            var marketId = splittedUrl[splittedUrl.length - 2];
            var indexOfVSuffix = marketId.indexOf('-v');
            if(indexOfVSuffix !== -1) {
                marketId = marketId.substring(0, indexOfVSuffix);
            }
            var avatarThumbnailUrl = "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/previews/%marketId%/medium/hifi-mp-%marketId%.jpg"
                .split('%marketId%').join(marketId);

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
}
