//
//  DataServerAccountInfo.h
//  hifi
//
//  Created by Stephen Birarda on 2/21/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DataServerAccountInfo__
#define __hifi__DataServerAccountInfo__

#include <QtCore/QObject>

#include "OAuthAccessToken.h"

class DataServerAccountInfo : public QObject {
    Q_OBJECT
public:
    DataServerAccountInfo();
    DataServerAccountInfo(const QJsonObject& jsonObject);
    DataServerAccountInfo(const DataServerAccountInfo& otherInfo);
    DataServerAccountInfo& operator=(const DataServerAccountInfo& otherInfo);
    
    const OAuthAccessToken& getAccessToken() const { return _accessToken; }
    
    const QString& getUsername() const { return _username; }
    void setUsername(const QString& username);
    
    friend QDataStream& operator<<(QDataStream &out, const DataServerAccountInfo& info);
    friend QDataStream& operator>>(QDataStream &in, DataServerAccountInfo& info);
private:
    void swap(DataServerAccountInfo& otherInfo);
    
    OAuthAccessToken _accessToken;
    QString _username;
};

#endif /* defined(__hifi__DataServerAccountInfo__) */
