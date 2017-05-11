//
// Created by Alan-Michael Moody on 4/17/2017
//

(function () {
    var barID;

    this.preload = function (entityID) {
        var children = Entities.getChildrenIDs(entityID);
        var childZero = Entities.getEntityProperties(children[0]);
        barID = childZero.id;
    };

    var SCAN_RATE = 100; //ms
    var REFERENCE_FRAME_COUNT = 30;
    var MAX_AUDIO_THRESHOLD = 16000;

    var framePool = [];

    function scanEngine() {
        var avatarLoudnessPool = [];

        function average(a) {
            var sum = 0;
            var total = a.length;
            for (var i = 0; i < total; i++) {
                sum += a[i];
            }
            return Math.round(sum / total);
        }

        function audioClamp(input) {
            if (input > MAX_AUDIO_THRESHOLD) return MAX_AUDIO_THRESHOLD;
            return input;
        }


        var avatars = AvatarList.getAvatarIdentifiers();
        avatars.forEach(function (id) {
            var avatar = AvatarList.getAvatar(id);
            avatarLoudnessPool.push(audioClamp(Math.round(avatar.audioLoudness)));
        });


        framePool.push(average(avatarLoudnessPool));
        if (framePool.length >= REFERENCE_FRAME_COUNT) {
            framePool.shift();
        }

        function normalizedAverage(a) {
            a = a.map(function (v) {
                return Math.round(( 100 / MAX_AUDIO_THRESHOLD ) * v);
            });
            return average(a);
        }

        var norm = normalizedAverage(framePool);


        var barProperties = Entities.getEntityProperties(barID);

        var colorShift = 2.55 * norm; //shifting the scale to 0 - 255
        var xShift = norm / 52; // changing scale from 0-100 to 0-1.9 ish
        var normShift = xShift - 0.88; //shifting local displacement (-0.88)
        var halfShift = xShift / 2;
        Entities.editEntity(barID, {
            dimensions: {x: xShift, y: barProperties.dimensions.y, z: barProperties.dimensions.z},
            localPosition: {x: normShift - (halfShift), y: -0.0625, z: -0.015},
            color: {red: colorShift, green: barProperties.color.green, blue: barProperties.color.blue}
        });


    }

    Script.setInterval(function () {
        scanEngine();
    }, SCAN_RATE);

});
