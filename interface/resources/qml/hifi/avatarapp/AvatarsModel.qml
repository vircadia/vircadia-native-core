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

    function arraysAreEqual(a1, a2, comparer) {
        if(Array.isArray(a1) && Array.isArray(a2)) {
            if(a1.length !== a2.length) {
                return false;
            }

            for(var i = 0; i < a1.length; ++i) {
                if(!comparer(a1[i], a2[i])) {
                    return false;
                }
            }
        } else if(Array.isArray(a1)) {
            return a1.length === 0;
        } else if(Array.isArray(a2)) {
            return a2.length === 0;
        }

        return true;
    }

    function compareArrays(a1, a2, props) {
        for(var prop in props) {
            if(JSON.stringify(a1[prop]) !== JSON.stringify(a2[prop])) {
                return false;
            }
        }

        return true;
    }

    function compareWearables(w1, w2) {
        return compareArrays(w1, w2, ['modelUrl', 'parentJointIndex', 'marketplaceID', 'itemName', 'script', 'rotation'])
    }

    function compareAttachments(a1, a2) {
        return compareAttachments(a1, a2, ['position', 'orientation', 'parentJointIndex', 'modelurl'])
    }

    function findAvatarIndexByValue(avatar) {

        var index = -1;
        var avatarObject = avatar.entry;

        // 2DO: find better way of determining selected avatar in bookmarks
        console.debug('allAvatars.count: ', allAvatars.count);
        for(var i = 0; i < allAvatars.count; ++i) {
            var thesame = true;
            var bookmarkedAvatarObject = allAvatars.get(i).entry;

            if(bookmarkedAvatarObject.avatarUrl !== avatarObject.avatarUrl)
                continue;

            if(bookmarkedAvatarObject.avatarScale !== avatarObject.avatarScale)
                continue;

            if(!arraysAreEqual(bookmarkedAvatarObject.attachments, avatarObject.attachments, compareAttachments)) {
                continue;
            }

            if(!arraysAreEqual(bookmarkedAvatarObject.avatarEntities, avatarObject.avatarEntities, compareWearables)) {
                continue;
            }

            if(thesame) {
                index = i;
                break;
            }
        }

        return index;
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
