//
//  AnimationServer.h
//  hifi
//
//  Created by Stephen Birarda on 12/5/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
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
