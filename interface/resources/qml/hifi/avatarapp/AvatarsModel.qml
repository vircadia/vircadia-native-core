import QtQuick 2.9

ListModel {
    id: model
    property url externalAvatarThumbnailUrl;

    function extractMarketId(avatarUrl) {
        var splittedUrl = avatarUrl.split('/');
        var marketId = splittedUrl[splittedUrl.length - 2];
        var indexOfVSuffix = marketId.indexOf('-v');
        if(indexOfVSuffix !== -1) {
            marketId = marketId.substring(0, indexOfVSuffix);
        }

        return marketId;
    }

    function makeMarketItemUrl(avatarUrl) {
        var marketItemUrl = "https://highfidelity.com/marketplace/items/%marketId%"
            .split('%marketId%').join(extractMarketId(avatarUrl));

        return marketItemUrl;
    }

    function makeThumbnailUrl(avatarUrl) {
        var avatarThumbnailUrl = "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/previews/%marketId%/large/hifi-mp-%marketId%.jpg"
            .split('%marketId%').join(extractMarketId(avatarUrl));

        return avatarThumbnailUrl;
    }

    function encodedName(avatarName, isExternal) {
        if(isExternal) {
            if(avatarName.indexOf('external:') !== 0) {
                return 'external:' + avatarName;
            }
        }

        return avatarName;
    }

    function decodedName(avatarName, isExternal) {
        if(isExternal) {
            if(avatarName.indexOf('external:') === 0) {
                avatarName = avatarName.replace('external:', '');
            }
        }

        return avatarName;
    }

    function makeAvatarObject(avatar, avatarName) {
        console.debug('makeAvatarEntry: ', avatarName, JSON.stringify(avatar));
        var isExternal = avatarName.indexOf('external:') === 0;
        avatarName = decodedName(avatarName, isExternal);

        var avatarThumbnailUrl = isExternal ? externalAvatarThumbnailUrl.toString() : makeThumbnailUrl(avatar.avatarUrl);
        console.debug('isExternal:', isExternal, 'avatarThumbnailUrl:', avatarThumbnailUrl, 'externalAvatarThumbnailUrl:', externalAvatarThumbnailUrl);

        return {
            'name' : avatarName,
            'avatarScale' : avatar.avatarScale,
            'thumbnailUrl' : avatarThumbnailUrl,
            'avatarUrl' : avatar.avatarUrl,
            'wearables' : avatar.avatarEntites ? avatar.avatarEntites : [],
            'attachments' : avatar.attachments ? avatar.attachments : [],
            'entry' : avatar,
            'isExternal' : isExternal,
            'getMoreAvatars' : false
        };

    }

    function addAvatarEntry(avatar, avatarName) {
        console.debug('addAvatarEntry: ', avatarName);

        var avatarEntry = makeAvatarObject(avatar, avatarName);
        append(avatarEntry);

        return allAvatars.count - 1;
    }

    function populate(bookmarks) {
        for(var avatarName in bookmarks) {
            var avatar = bookmarks[avatarName];
            var avatarEntry = makeAvatarObject(avatar, avatarName);

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

            var allDifferent = true;

            // it turns out order of wearables can randomly change so make position-independent comparison here
            for(var j = 0; j < m2.count; ++j) {
                var e2 = m2.get(j);

                if(comparer(e1, e2)) {
                    allDifferent = false;
                    break;
                }
            }

            if(allDifferent) {
                return false;
            }
        }

        return true;
    }

    function compareNumericObjects(o1, o2) {
        if(o1 === undefined && o2 !== undefined)
            return false;
        if(o1 !== undefined && o2 === undefined)
            return false;

        for(var prop in o1) {
            var v1 = o1[prop];
            var v2 = o2[prop];

            if(v1 !== v2 && Math.round(v1 * 1000) != Math.round(v2 * 1000))
                return false;
        }

        return true;
    }

    function compareObjects(o1, o2, props, arrayProp) {

        console.debug('compare ojects: o1 = ', JSON.stringify(o1, null, 4), 'o2 = ', JSON.stringify(o2, null, 4));
        for(var i = 0; i < props.length; ++i) {
            var prop = props[i];
            var propertyName = prop.propertyName;
            var comparer = prop.comparer;

            var o1Value = arrayProp ? o1[arrayProp][propertyName] : o1[propertyName];
            var o2Value = arrayProp ? o2[arrayProp][propertyName] : o2[propertyName];

            console.debug('compare values for key: ', propertyName, comparer ? 'using specified comparer' : 'using default comparer', ', o1 = ', JSON.stringify(o1Value, null, 4), 'o2 = ', JSON.stringify(o2Value, null, 4));

            if(comparer) {
                if(comparer(o1Value, o2Value) === false) {
                    console.debug('not equal');
                    return false;
                } else {
                    console.debug('equal');
                }
            } else {
                if(JSON.stringify(o1Value) !== JSON.stringify(o2Value)) {
                    return false;
                }
            }
        }

        return true;
    }

    function compareWearables(w1, w2) {
        return compareObjects(w1, w2, [{'propertyName' : 'modelURL'},
                                       {'propertyName' : 'parentJointIndex'},
                                       {'propertyName' : 'marketplaceID'},
                                       {'propertyName' : 'itemName'},
                                       {'propertyName' : 'script'},
                                       {'propertyName' : 'rotation', 'comparer' : compareNumericObjects},
                                       {'propertyName' : 'localPosition', 'comparer' : compareNumericObjects},
                                       {'propertyName' : 'localRotation', 'comparer' : compareNumericObjects},
                                       {'propertyName' : 'dimensions', 'comparer' : compareNumericObjects}], 'properties')
    }

    function compareAttachments(a1, a2) {
        return compareObjects(a1, a2, [{'propertyName' : 'position', 'comparer' : compareNumericObjects},
                                       {'propertyName' : 'orientation'},
                                       {'propertyName' : 'parentJointIndex'},
                                       {'propertyName' : 'modelurl'}])
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
