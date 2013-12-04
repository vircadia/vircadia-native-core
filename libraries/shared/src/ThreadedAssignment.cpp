//
//  ThreadedAssignment.cpp
//  hifi
//
//  Created by Stephen Birarda on 12/3/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "ThreadedAssignment.h"

ThreadedAssignment::ThreadedAssignment(const unsigned char* dataBuffer, int numBytes) :
    Assignment(dataBuffer, numBytes),
    _isFinished(false)
{
    
}

void ThreadedAssignment::setFinished(bool isFinished) {
    _isFinished = isFinished;

    if (_isFinished) {
        emit finished();
    }
}

void ThreadedAssignment::checkInWithDomainServerOrExit() {
    if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
        setFinished(true);
    } else {
        NodeList::getInstance()->sendDomainServerCheckIn();
    }
}
