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
