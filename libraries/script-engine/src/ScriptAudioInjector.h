//
//  ScriptAudioInjector.h
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2015-02-11.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptAudioInjector_h
#define hifi_ScriptAudioInjector_h

#include <QtCore/QObject>

#include <AudioInjector.h>

/**jsdoc
 * Plays &mdash; "injects" &mdash; the content of an audio file. Used in the {@link Audio} API.
 *
 * @class AudioInjector
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-server-entity
 * @hifi-assignment-client
 *
 * @property {boolean} playing - <code>true</code> if the audio is currently playing, otherwise <code>false</code>. 
 *     <em>Read-only.</em>
 * @property {number} loudness - The loudness in the last frame of audio, range <code>0.0</code> &ndash; <code>1.0</code>. 
 *     <em>Read-only.</em>
 * @property {AudioInjector.AudioInjectorOptions} options - Configures how the injector plays the audio.
 */
class ScriptAudioInjector : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool playing READ isPlaying)
    Q_PROPERTY(float loudness READ getLoudness)
    Q_PROPERTY(AudioInjectorOptions options WRITE setOptions READ getOptions)
public:
    ScriptAudioInjector(const AudioInjectorPointer& injector);
    ~ScriptAudioInjector();
public slots:

    /**jsdoc
     * Stop current playback, if any, and start playing from the beginning.
     * @function AudioInjector.restart
     */
    void restart() { _injector->restart(); }

    /**jsdoc
     * Stop audio playback.
     * @function AudioInjector.stop
     * @example <caption>Stop playing a sound before it finishes.</caption>
     * var sound = SoundCache.getSound(Script.resourcesPath() + "sounds/sample.wav");
     * var injector;
     * var injectorOptions = {
     *     position: MyAvatar.position
     * };
     * 
     * Script.setTimeout(function () { // Give the sound time to load.
     *     injector = Audio.playSound(sound, injectorOptions);
     * }, 1000);
     * 
     * Script.setTimeout(function () {
     *     injector.stop();
     * }, 2000);
     */
    void stop() { _injector->stop(); }

    /**jsdoc
     * Get the current configuration of the audio injector.
     * @function AudioInjector.getOptions
     * @returns {AudioInjector.AudioInjectorOptions} Configuration of how the injector plays the audio.
     */
    const AudioInjectorOptions& getOptions() const { return _injector->getOptions(); }

    /**jsdoc
     * Configure how the injector plays the audio.
     * @function AudioInjector.setOptions
     * @param {AudioInjector.AudioInjectorOptions} options - Configuration of how the injector plays the audio.
     */
    void setOptions(const AudioInjectorOptions& options) { _injector->setOptions(options); }

    /**jsdoc
     * Get the loudness of the most recent frame of audio played.
     * @function AudioInjector.getLoudness
     * @returns {number} The loudness of the most recent frame of audio played, range <code>0.0</code> &ndash; <code>1.0</code>.
     */
    float getLoudness() const { return _injector->getLoudness(); }

    /**jsdoc
     * Get whether or not the audio is currently playing.
     * @function AudioInjector.isPlaying
     * @returns {boolean} <code>true</code> if the audio is currently playing, otherwise <code>false</code>.
     * @example <caption>See if a sound is playing.</caption>
     * var sound = SoundCache.getSound(Script.resourcesPath() + "sounds/sample.wav");
     * var injector;
     * var injectorOptions = {
     *     position: MyAvatar.position
     * };
     *
     * Script.setTimeout(function () { // Give the sound time to load.
     *     injector = Audio.playSound(sound, injectorOptions);
     * }, 1000);
     *
     * Script.setTimeout(function () {
     *     print("Sound is playing: " + injector.isPlaying());
     * }, 2000);
     */
    bool isPlaying() const { return _injector->isPlaying(); }

signals:

    /**jsdoc
     * Triggered when the audio has finished playing.
     * @function AudioInjector.finished
     * @returns {Signal}
     * @example <caption>Report when a sound has finished playing.</caption>
     * var sound = SoundCache.getSound(Script.resourcesPath() + "sounds/sample.wav");
     * var injector;
     * var injectorOptions = {
     *     position: MyAvatar.position
     * };
     * 
     * Script.setTimeout(function () { // Give the sound time to load.
     *     injector = Audio.playSound(sound, injectorOptions);
     *     injector.finished.connect(function () {
     *         print("Finished playing sound");
     *     });
     * }, 1000);
     */
    void finished();

protected slots:

    /**jsdoc
     * Stop audio playback. (Synonym of {@link AudioInjector.stop|stop}.)
     * @function AudioInjector.stopInjectorImmediately
     */
    void stopInjectorImmediately();
private:
    AudioInjectorPointer _injector;

    friend QScriptValue injectorToScriptValue(QScriptEngine* engine, ScriptAudioInjector* const& in);
};

Q_DECLARE_METATYPE(ScriptAudioInjector*)

QScriptValue injectorToScriptValue(QScriptEngine* engine, ScriptAudioInjector* const& in);
void injectorFromScriptValue(const QScriptValue& object, ScriptAudioInjector*& out);

#endif // hifi_ScriptAudioInjector_h
