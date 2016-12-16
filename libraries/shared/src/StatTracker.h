//
//  Created by Ryan Huffman on 2016-12-14
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <mutex>
#include "DependencyManager.h"
#include "Trace.h"

using EditStatFunction = std::function<QVariant(QVariant currentValue)>;

class StatTracker : public Dependency {
public:
    StatTracker();
    QVariant getStat(QString name);
    void editStat(QString name, EditStatFunction fn);
    void incrementStat(QString name);
    void decrementStat(QString name);
private:
    std::mutex _statsLock;
    QVariantMap _stats;
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
