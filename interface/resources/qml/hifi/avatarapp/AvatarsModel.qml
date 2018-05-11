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
            var avatar = bookmarks[avatarName];
            var avatarThumbnailUrl = makeThumbnailUrl(avatar.avatarUrl);

            var avatarEntry = {
                'name' : avatarName,
                'scale' : avatar.avatarScale,
                'url' : avatarThumbnailUrl,
                'wearables' : avatar.avatarEntites ? avatar.avatarEntites : [],
                'attachments' : avatar.attachments ? avatar.attachments : [],
                'entry' : avatar,
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

    function modelsAreEqual(m1, m2, comparer) {
        if(m1.count !== m2.count) {
            return false;
        }

        for(var i = 0; i < m1.count; ++i) {
            var e1 = m1.get(i);
            var e2 = m2.get(i);

            console.debug('comparing ', JSON.stringify(e1), JSON.stringify(e2));

            if(!comparer(e1, e2)) {
                return false;
            }
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
        console.debug('findAvatarIndexByValue: ', JSON.stringify(avatar));

        // 2DO: find better way of determining selected avatar in bookmarks
        console.debug('allAvatars.count: ', allAvatars.count);
        for(var i = 0; i < allAvatars.count; ++i) {
            var thesame = true;
            var bookmarkedAvatar = allAvatars.get(i);

            if(bookmarkedAvatar.avatarUrl !== avatar.avatarUrl)
                continue;

            if(bookmarkedAvatar.avatarScale !== avatar.avatarScale)
                continue;

            if(!modelsAreEqual(bookmarkedAvatar.attachments, avatar.attachments, compareAttachments)) {
                continue;
            }

            if(!modelsAreEqual(bookmarkedAvatar.wearables, avatar.wearables, compareWearables)) {
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
