/**
 * An array for sounds, allows you to randomly play a sound
 * taken from the removed editVoxels.js
 */
SoundArray = function(audioOptions, autoUpdateAudioPosition) {
    this.audioOptions = audioOptions !== undefined ? audioOptions : {};
    this.autoUpdateAudioPosition = autoUpdateAudioPosition !== undefined ? autoUpdateAudioPosition : false;
    if (this.audioOptions.position === undefined) {
        this.audioOptions.position = Vec3.sum(MyAvatar.position, { x: 0, y: 1, z: 0});
    }
    if (this.audioOptions.volume === undefined) {
        this.audioOptions.volume = 1.0;
    }
    this.sounds = new Array();
    this.addSound = function (soundURL) {
        this.sounds[this.sounds.length] = SoundCache.getSound(soundURL);
    };
    this.play = function (index) {
        if (0 <= index && index < this.sounds.length) {
            if (this.autoUpdateAudioPosition) {
                this.updateAudioPosition();
            }
            if (this.sounds[index].downloaded) {
                Audio.playSound(this.sounds[index], this.audioOptions);
            }
        } else {
            print("[ERROR] libraries/soundArray.js:play() : Index " + index + " out of range.");
        }
    };
    this.playRandom = function () {
        if (this.sounds.length > 0) {
            this.play(Math.floor(Math.random() * this.sounds.length));
        } else {
            print("[ERROR] libraries/soundArray.js:playRandom() : Array is empty.");
        }
    };
    this.updateAudioPosition = function() {
        var position = MyAvatar.position;
        var forwardVector = Quat.getForward(MyAvatar.orientation);
        this.audioOptions.position = Vec3.sum(position, forwardVector);
    };
};
