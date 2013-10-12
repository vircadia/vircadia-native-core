//
//  Agent.h
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Agent__
#define __hifi__Agent__

#include <vector>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>
#include <QtCore/QUrl>

#include <AudioInjector.h>
#include <Assignment.h>

class Agent : public Assignment {
    Q_OBJECT
public:
    Agent(const unsigned char* dataBuffer, int numBytes);
    
    void run();
public slots:
    void stop();
signals:
    void willSendAudioDataCallback();
    void willSendVisualDataCallback();
private:
    static void setStaticInstance(Agent* staticInstance);
    static Agent* _staticInstance;
    static QScriptValue AudioInjectorConstructor(QScriptContext *context, QScriptEngine *engine);
    
    bool volatile _shouldStop;
    std::vector<AudioInjector*> _audioInjectors;
};

#endif /* defined(__hifi__Operative__) */
