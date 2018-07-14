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
 * TODO
 * Used in the {@link Audio} API.
 *
 * @class AudioInjector
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-server-entity
 * @hifi-assignment-client
 *
 * @property {boolean} playing - <code>true</code> if TODO, otherwise <code>false</code>. <em>Read-only.</em>
 * @property {number} loudness - TODO <em>Read-only.</em>
 * @property {AudioInjector.AudioInjectorOptions} options - TODO
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
     * TODO
     * @function AudioInjector.restart
     */
    void restart() { _injector->restart(); }

    /**jsdoc
     * TODO
     * @function AudioInjector.stop
     */
    void stop() { _injector->stop(); }

    /**jsdoc
     * TODO
     * @function AudioInjector.getOptions
     * @returns {AudioInjector.AudioInjectorOptions} TODO
     */
    const AudioInjectorOptions& getOptions() const { return _injector->getOptions(); }

    /**jsdoc
     * TODO
     * @function AudioInjector.setOptions
     * @param {AudioInjector.AudioInjectorOptions} options - TODO
     */
    void setOptions(const AudioInjectorOptions& options) { _injector->setOptions(options); }

    /**jsdoc
     * TODO
     * @function AudioInjector.getLoudness
     * @returns {number} TODO
     */
    float getLoudness() const { return _injector->getLoudness(); }

    /**jsdoc
     * TODO
     * @function AudioInjector.isPlaying
     * @returns {boolean} <code>true</code> if TODO, otherwise <code>false</code>.
     */
    bool isPlaying() const { return _injector->isPlaying(); }

signals:

    /**jsdoc
     * Triggered when TODO
     * @function AudioInjector.finished
     * @returns {Signal}
     */
    void finished();

protected slots:

    /**jsdoc
     * TODO
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
