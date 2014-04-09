//
//  AnimationServer.h
//  animation-server/src
//
//  Created by Stephen Birarda on 12/5/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__AnimationServer__
#define __hifi__AnimationServer__

#include <QtCore/QCoreApplication>

class AnimationServer : public QCoreApplication {
    Q_OBJECT
public:
    AnimationServer(int &argc, char **argv);
    ~AnimationServer();
private slots:
    void readPendingDatagrams();
};


#endif /* defined(__hifi__AnimationServer__) */
