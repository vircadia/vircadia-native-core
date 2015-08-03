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

#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>
#include <QtScript/qscriptengine.h>

#include <ResourceCache.h>

class Sound : public Resource {
    Q_OBJECT
    
    Q_PROPERTY(bool downloaded READ isReady)
public:
    Sound(const QUrl& url, bool isStereo = false);
    
    bool isStereo() const { return _isStereo; }    
    bool isReady() const { return _isReady; }
     
    const QByteArray& getByteArray() { return _byteArray; }

private:
    QByteArray _byteArray;
    bool _isStereo;
    bool _isReady;
    
    void trimFrames();
    void downSample(const QByteArray& rawAudioByteArray);
    void interpretAsWav(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray);
    
    virtual void downloadFinished(const QByteArray& data) override;
};

typedef QSharedPointer<Sound> SharedSoundPointer;

Q_DECLARE_METATYPE(SharedSoundPointer)
QScriptValue soundSharedPointerToScriptValue(QScriptEngine* engine, SharedSoundPointer const& in);
void soundSharedPointerFromScriptValue(const QScriptValue& object, SharedSoundPointer &out);

Q_DECLARE_METATYPE(Sound*)
QScriptValue soundPointerToScriptValue(QScriptEngine* engine, Sound* const& in);
void soundPointerFromScriptValue(const QScriptValue& object, Sound* &out);


#endif // hifi_Sound_h
