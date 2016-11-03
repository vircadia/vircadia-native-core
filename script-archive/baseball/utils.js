//
//  utils.js
//  examples/baseball/
//
//  Created by Ryan Huffman on Nov 9, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

randomInt = function(low, high) {
    return Math.floor(randomFloat(low, high));
};

randomFloat = function(low, high) {
    if (high === undefined) {
        high = low;
        low = 0;
    }
    return low + Math.random() * (high - low);
};

randomColor = function(redMin, redMax, greenMin, greenMax, blueMin, blueMax) {
    return {
        red: Math.ceil(randomFloat(redMin, redMax)),
        green: Math.ceil(randomFloat(greenMin, greenMax)),
        blue: Math.ceil(randomFloat(blueMin, blueMax)),
    }
};

randomVec3 = function(xMin, xMax, yMin, yMax, zMin, zMax) {
    return {
        x: randomFloat(xMin, xMax),
        y: randomFloat(yMin, yMax),
        z: randomFloat(zMin, zMax),
    }
};

getSounds = function(soundURLs) {
    var sounds = [];
    for (var i = 0; i < soundURLs.length; ++i) {
        sounds.push(SoundCache.getSound(soundURLs[i], false));
    }
    return sounds;
};

playRandomSound = function(sounds, options) {
    if (options === undefined) {
        options = {
            volume: 1.0,
            position: MyAvatar.position,
        }
    }
    return Audio.playSound(sounds[randomInt(sounds.length)], options);
}

shallowCopy = function(obj) {
    var copy = {}
    for (var key in obj) {
        copy[key] = obj[key];
    }
    return copy;
}

findEntity = function(properties, searchRadius) {
    var entities = findEntities(properties, searchRadius);
    return entities.length > 0 ? entities[0] : null;
}

// Return all entities with properties `properties` within radius `searchRadius`
findEntities = function(properties, searchRadius) {
    var entities = Entities.findEntities(MyAvatar.position, searchRadius);
    var matchedEntities = [];
    var keys = Object.keys(properties);
    for (var i = 0; i < entities.length; ++i) {
        var match = true;
        var candidateProperties = Entities.getEntityProperties(entities[i], keys);
        for (var key in properties) {
            if (candidateProperties[key] != properties[key]) {
                // This isn't a match, move to next entity
                match = false;
                break;
            }
        }
        if (match) {
            matchedEntities.push(entities[i]);
        }
    }

    return matchedEntities;
}
