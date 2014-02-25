//
//  DataServerAccountInfo.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QDebug>

#include "DataServerAccountInfo.h"

DataServerAccountInfo::DataServerAccountInfo() :
    _accessToken(),
    _username()
{
    
}

DataServerAccountInfo::DataServerAccountInfo(const QJsonObject& jsonObject) :
    _accessToken(jsonObject),
    _username()
{
    setUsername(jsonObject["user"].toObject()["username"].toString());
}

DataServerAccountInfo::DataServerAccountInfo(const DataServerAccountInfo& otherInfo) {
    _accessToken = otherInfo._accessToken;
    _username = otherInfo._username;
}

DataServerAccountInfo& DataServerAccountInfo::operator=(const DataServerAccountInfo& otherInfo) {
    DataServerAccountInfo temp(otherInfo);
    swap(temp);
    return *this;
}

void DataServerAccountInfo::swap(DataServerAccountInfo& otherInfo) {
    using std::swap;
    
    swap(_accessToken, otherInfo._accessToken);
    swap(_username, otherInfo._username);
}

void DataServerAccountInfo::setUsername(const QString& username) {
    if (_username != username) {
        _username = username;
        
        qDebug() << "Username changed to" << username;
    }
}

QDataStream& operator<<(QDataStream &out, const DataServerAccountInfo& info) {
    out << info._accessToken << info._username;
    return out;
}

QDataStream& operator>>(QDataStream &in, DataServerAccountInfo& info) {
    in >> info._accessToken >> info._username;
    return in;
}