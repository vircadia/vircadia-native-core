//
//  Sound.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Sound_h
#define hifi_Sound_h

#include <QRunnable>
#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>
#include <QtScript/qscriptengine.h>

#include <ResourceCache.h>

class Sound : public Resource {
    Q_OBJECT

public:
    Sound(const QUrl& url, bool isStereo = false, bool isAmbisonic = false);
    
    bool isStereo() const { return _isStereo; }    
    bool isAmbisonic() const { return _isAmbisonic; }    
    bool isReady() const { return _isReady; }
    float getDuration() const { return _duration; }
 
    const QByteArray& getByteArray() const { return _byteArray; }

signals:
    void ready();

protected slots:
    void soundProcessSuccess(QByteArray data, bool stereo, bool ambisonic, float duration);
    void soundProcessError(int error, QString str);
    
private:
    QByteArray _byteArray;
    bool _isStereo;
    bool _isAmbisonic;
    bool _isReady;
    float _duration; // In seconds
    
    virtual void downloadFinished(const QByteArray& data) override;
};

class SoundProcessor : public QObject, public QRunnable {
    Q_OBJECT

public:
    SoundProcessor(const QUrl& url, const QByteArray& data, bool stereo, bool ambisonic)
        : _url(url), _data(data), _isStereo(stereo), _isAmbisonic(ambisonic)
    {
    }

    virtual void run() override;

    void downSample(const QByteArray& rawAudioByteArray, int sampleRate);
    int interpretAsWav(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray);
    int interpretAsMP3(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray);

signals:
    void onSuccess(QByteArray data, bool stereo, bool ambisonic, float duration);
    void onError(int error, QString str);

private:
    QUrl _url;
    QByteArray _data;
    bool _isStereo;
    bool _isAmbisonic;
    float _duration;
};

typedef QSharedPointer<Sound> SharedSoundPointer;

/**jsdoc
 * An audio resource, created by {@link SoundCache.getSound}, to be played back using {@link Audio.playSound}.
 * <p>Supported formats:</p>
 * <ul>
 *   <li>WAV: 16-bit uncompressed WAV at any sample rate, with 1 (mono), 2(stereo), or 4 (ambisonic) channels.</li>
 *   <li>MP3: Mono or stereo, at any sample rate.</li>
 *   <li>RAW: 48khz 16-bit mono or stereo. Filename must include <code>".stereo"</code> to be interpreted as stereo.</li>
 * </ul>
 *
 * @class SoundObject
 * 
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-server-entity
 * @hifi-assignment-client
 *
 * @property {boolean} downloaded - <code>true</code> if the sound has been downloaded and is ready to be played, otherwise 
 *     <code>false</code>.
 * @property {number} duration - The duration of the sound, in seconds.
 */
class SoundScriptingInterface : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool downloaded READ isReady)
    Q_PROPERTY(float duration READ getDuration)

public:
    SoundScriptingInterface(SharedSoundPointer sound);
    SharedSoundPointer getSound() { return _sound; }

    bool isReady() const { return _sound->isReady(); }
    float getDuration() { return _sound->getDuration(); }

/**jsdoc
 * Triggered when the sound has been downloaded and is ready to be played.
 * @function SoundObject.ready
 * @returns {Signal}
 */
signals:
    void ready();

private:
    SharedSoundPointer _sound;
};

Q_DECLARE_METATYPE(SharedSoundPointer)
QScriptValue soundSharedPointerToScriptValue(QScriptEngine* engine, const SharedSoundPointer& in);
void soundSharedPointerFromScriptValue(const QScriptValue& object, SharedSoundPointer& out);

#endif // hifi_Sound_h
