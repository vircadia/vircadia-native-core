//
//  ConnectionMonitor.h
//  interface/src
//
//  Created by Ryan Huffman on 8/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ConnectionMonitor_h
#define hifi_ConnectionMonitor_h

#include <QObject>
#include <QTimer>

class QUrl;
class QString;

class ConnectionMonitor : public QObject {
    Q_OBJECT
public:
    void init();

signals:
    void setRedirectErrorState(QUrl errorURL, int reasonCode);

private slots:
    void startTimer();
    void stopTimer();

private:
    QTimer _timer;
};

#endif // hifi_ConnectionMonitor_h