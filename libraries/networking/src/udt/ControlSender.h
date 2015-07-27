//
//  ControlSender.h
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-27.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_ControlSender_h
#define hifi_ControlSender_h

#include <QtCore/QObject>

namespace udt {

// Handles the sending of periodic control packets for all active UDT reliable connections
// Currently the interval for all connections is the same, so one thread is sufficient to manage all
class ControlSender : public QObject {
    Q_OBJECT
public:
    ControlSender(QObject* parent = 0) : QObject(parent) {};
    
public slots:
    void loop(); // infinitely loops and sleeps to manage rate of control packet sending
    void stop() { _isStopped = true; } // stops the loop
    
private:
    int _synInterval = 10 * 1000;
    bool _isStopped { false };
};
    
}

#endif // hifi_ControlSender_h
