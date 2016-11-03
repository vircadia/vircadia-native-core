//
//  GPUIdent.h
//  libraries/shared/src
//
//  Provides information about the GPU
//
//  Created by Howard Stearns on 4/16/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GPUIdent_h
#define hifi_GPUIdent_h

#include <cstdint>

class GPUIdent
{
public:
    uint64_t getMemory() { return _dedicatedMemoryMB; }
    QString getName() { return _name; }
    QString getDriver() { return _driver; }
    bool isValid() { return _isValid; }
    // E.g., GPUIdent::getInstance()->getMemory();
    static GPUIdent* getInstance(const QString& vendor = "", const QString& renderer = "") { return _instance.ensureQuery(vendor, renderer); }
private:
    uint64_t _dedicatedMemoryMB { 0 };
    QString _name { "" };
    QString _driver { "" };
    bool _isQueried { false };
    bool _isValid { false };
    static GPUIdent _instance;
    GPUIdent* ensureQuery(const QString& vendor, const QString& renderer);
};

#endif // hifi_GPUIdent_h
