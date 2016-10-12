//
//  AvatarAudioTimer.h
//  assignment-client/src
//
//  Created by David Kelly on 10/12/13.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarAudioTimer_h
#define hifi_AvatarAudioTimer_h

#include <QtCore/QObject>

class AvatarAudioTimer : public QObject {
    Q_OBJECT

signals:
    void avatarTick();

public slots:
    void start();
    void stop() { _quit = true; }

private:
    bool _quit { false };
};

#endif //hifi_AvatarAudioTimer_h
