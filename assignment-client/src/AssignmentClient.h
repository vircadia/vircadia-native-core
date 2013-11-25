//
//  AssignmentClient.h
//  hifi
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AssignmentClient__
#define __hifi__AssignmentClient__

#include <QtCore/QCoreApplication>

class AssignmentClient : public QCoreApplication {
    Q_OBJECT
public:
    AssignmentClient(int &argc, char **argv,
                     Assignment::Type requestAssignmentType = Assignment::AllTypes,
                     const sockaddr_in& customAssignmentServerSocket = sockaddr_in(),
                     const char* requestAssignmentPool = NULL);
    
    int exec();
private:
    Assignment::Type _requestAssignmentType;
    sockaddr_in _customAssignmentServerSocket;
    const char* _requestAssignmentPool;
};

#endif /* defined(__hifi__AssignmentClient__) */
