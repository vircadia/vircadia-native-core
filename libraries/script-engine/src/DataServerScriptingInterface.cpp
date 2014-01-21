//
//  DataServerScriptingInterface.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/20/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <DataServerClient.h>

#include "DataServerScriptingInterface.h"

DataServerScriptingInterface::DataServerScriptingInterface() :
    _uuid(QUuid::createUuid())
{
    
}

void DataServerScriptingInterface::setValueForKey(const QString& key, const QString& value) {
    DataServerClient::putValueForKeyAndUUID(key, value, _uuid);
}