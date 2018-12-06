//
//  AudioSolo.h
//  libraries/audio/src
//
//  Created by Clement Brisset on 11/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_AudioSolo_h
#define hifi_AudioSolo_h

#include <mutex>

#include <QSet>
#include <QUuid>

class AudioSolo {
    using Mutex = std::recursive_mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    bool isSoloing() const;
    QVector<QUuid> getUUIDs() const;
    void addUUIDs(QVector<QUuid> uuidList);
    void removeUUIDs(QVector<QUuid> uuidList);
    void reset();

    void resend();

private:
    mutable Mutex _mutex;
    QSet<QUuid> _nodesSoloed;
};

#endif // hifi_AudioSolo_h
