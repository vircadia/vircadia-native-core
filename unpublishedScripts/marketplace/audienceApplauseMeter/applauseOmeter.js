//
// Created by Alan-Michael Moody on 5/2/2017
//

(function () {
    var thisEntityID;

    this.preload = function (entityID) {
        thisEntityID = entityID;
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

        // we have a range of 55 to -53 degrees for the needle

        var scaledDegrees = (norm / -.94) + 54.5; // shifting scale from 100 to 55 to -53 ish its more like -51 ;

        Entities.setAbsoluteJointRotationInObjectFrame(thisEntityID, 0, Quat.fromPitchYawRollDegrees(0, 0, scaledDegrees));

    }

    Script.setInterval(function () {
        scanEngine();
    }, SCAN_RATE);

});
