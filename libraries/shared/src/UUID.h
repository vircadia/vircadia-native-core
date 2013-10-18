//
//  UUID.h
//  hifi
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__UUID__
#define __hifi__UUID__

#include <QtCore/QUuid>

const int NUM_BYTES_RFC4122_UUID = 16;

QString uuidStringWithoutCurlyBraces(const QUuid& uuid);

#endif /* defined(__hifi__UUID__) */
