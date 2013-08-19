//
//  PairingHandler.h
//  hifi
//
//  Created by Stephen Birarda on 5/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__PairingHandler__
#define __hifi__PairingHandler__

#include <QtCore/QObject>

class PairingHandler : public QObject {
    Q_OBJECT
public:
    static PairingHandler* getInstance();
public slots:
    void sendPairRequest();
};

#endif /* defined(__hifi__PairingHandler__) */
