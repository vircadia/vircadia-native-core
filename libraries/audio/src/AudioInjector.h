//
//  AudioInjector.h
//  hifi
//
//  Created by Stephen Birarda on 12/19/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjector__
#define __hifi__AudioInjector__

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QUrl>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class AbstractAudioInterface;
class QNetworkReply;

const uchar MAX_INJECTOR_VOLUME = 0xFF;

class AudioInjector : public QObject {
    Q_OBJECT
public:
    AudioInjector(const QUrl& sampleURL);
    
    int size() const { return _sampleByteArray.size(); }
    
    void setPosition(const glm::vec3& position) { _position = position; }
    void setOrientation(const glm::quat& orientation) { _orientation = orientation; }
    void setVolume(float volume) { _volume = std::max(fabsf(volume), 1.0f); }
public slots:
    void injectViaThread(AbstractAudioInterface* localAudioInterface = NULL);
    
private:
    QByteArray _sampleByteArray;
    int _currentSendPosition;
    QThread _thread;
    QUrl _sourceURL;
    glm::vec3 _position;
    glm::quat _orientation;
    float _volume;
    
private slots:
    void startDownload();
    void replyFinished(QNetworkReply* reply);
    void injectAudio(AbstractAudioInterface* localAudioInterface);
};

#endif /* defined(__hifi__AudioInjector__) */
