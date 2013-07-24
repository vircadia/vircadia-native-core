//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#import <QtScript/QScriptEngine>
#import <QtNetwork/QtNetwork>

#include "AvatarData.h"

#include "Agent.h"

void Agent::run(QUrl scriptURL) {
    
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    
    qDebug() << "Attemping download of " << scriptURL;
    
    QNetworkReply* reply = manager->get(QNetworkRequest(scriptURL));
    
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    
    QString scriptString = QString(reply->readAll());
    
    QScriptEngine engine;
    
    AvatarData *someObject = new AvatarData;
    
    QScriptValue objectValue = engine.newObject();
    engine.globalObject().setProperty("AvatarData", objectValue);
    
    qDebug() << "Execution of script:" << engine.evaluate(scriptString).toString();
}