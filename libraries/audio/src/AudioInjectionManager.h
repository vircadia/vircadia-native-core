//
//  AudioInjectionManager.h
//  hifi
//
//  Created by Stephen Birarda on 12/17/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjectionManager__
#define __hifi__AudioInjectionManager__

#include <QtCore/QObject>

class AudioInjectionManager : public QObject {
    AudioInjectionManager(QObject* parent = 0);
};

#endif /* defined(__hifi__AudioInjectionManager__) */
