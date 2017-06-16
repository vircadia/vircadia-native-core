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

class SoundScriptingInterface : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool downloaded READ isReady)
    Q_PROPERTY(float duration READ getDuration)

public:
    SoundScriptingInterface(SharedSoundPointer sound);
    SharedSoundPointer getSound() { return _sound; }

    bool isReady() const { return _sound->isReady(); }
    float getDuration() { return _sound->getDuration(); }

signals:
    void ready();

private:
    SharedSoundPointer _sound;
};

Q_DECLARE_METATYPE(SharedSoundPointer)
QScriptValue soundSharedPointerToScriptValue(QScriptEngine* engine, const SharedSoundPointer& in);
void soundSharedPointerFromScriptValue(const QScriptValue& object, SharedSoundPointer& out);

#endif // hifi_Sound_h
