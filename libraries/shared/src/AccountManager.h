//
//  AccountManager.h
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AccountManager__
#define __hifi__AccountManager__

#include <QtCore/QByteArray>

class AccountManager {
public:
    static void processDomainServerAuthRequest(const QByteArray& packet);
    
    static const QString& getUsername() { return _username; }
    static void setUsername(const QString& username) { _username = username; }
private:
    static QString _username;
};

#endif /* defined(__hifi__AccountManager__) */
