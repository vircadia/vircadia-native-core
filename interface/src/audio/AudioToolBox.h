//
//  AudioToolBox.h
//  interface/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioToolBox_h
#define hifi_AudioToolBox_h

#include <DependencyManager.h>

class AudioToolBox : public QObject {
    SINGLETON_DEPENDENCY(AudioToolBox)
public:
    void render(int x, int y, bool boxed);
    
    bool mousePressEvent(int x, int y);
protected:
    AudioToolBox();
private:
    GLuint _micTextureId;
    GLuint _muteTextureId;
    GLuint _boxTextureId;
    QRect _iconBounds;
    qint64 _iconPulseTimeReference;
};

#endif // hifi_AudioToolBox_h