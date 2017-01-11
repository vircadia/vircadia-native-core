//
//  Created by Ryan Huffman on 2016-12-14
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QSet>
#include <QtCore/QVariantMap>

#include <mutex>

#include "DependencyManager.h"
#include "Trace.h"

using EditStatFunction = std::function<QVariant(QVariant currentValue)>;

class StatTracker : public Dependency {
public:
    StatTracker();
    QVariant getStat(const QString& name);
    void setStat(const QString& name, int value);
    void updateStat(const QString& name, int mod);
    void incrementStat(const QString& name);
    void decrementStat(const QString& name);
private:
    using Mutex = std::mutex;
    using Lock = std::lock_guard<Mutex>;
    Mutex _statsLock;
    QHash<QString, int> _stats;
};

class CounterStat {
public:
    CounterStat(QString name) : _name(name) {
        DependencyManager::get<StatTracker>()->incrementStat(_name);
    }    
    ~CounterStat() {
        DependencyManager::get<StatTracker>()->decrementStat(_name);
    }    
private:
    QString _name;
};
