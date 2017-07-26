//
//  Created by Anthony J. Thibault on 2017-01-30
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SoundEffect_h
#define hifi_SoundEffect_h

#include <QObject>
#include <QQuickItem>

#include <SoundCache.h>

class AudioInjector;
using AudioInjectorPointer = QSharedPointer<AudioInjector>;

// SoundEffect object, exposed to qml only, not interface JavaScript.
// This is used to play spatial sound effects on tablets/web entities from within QML.

class SoundEffect : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QUrl source READ getSource WRITE setSource)
    Q_PROPERTY(float volume READ getVolume WRITE setVolume)
public:

    virtual ~SoundEffect();

    QUrl getSource() const;
    void setSource(QUrl url);

    float getVolume() const;
    void setVolume(float volume);

    Q_INVOKABLE void play(QVariant position);
protected:
    QUrl _url;
    float _volume { 1.0f };
    SharedSoundPointer _sound;
    AudioInjectorPointer _injector;
};

#endif  // hifi_SoundEffect_h
