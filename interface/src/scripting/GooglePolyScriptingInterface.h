//
//  GooglePolyScriptingInterface.h
//  interface/src/scripting
//
//  Created by Elisa Lupin-Jimenez on 12/3/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GooglePolyScriptingInterface_h
#define hifi_GooglePolyScriptingInterface_h

#include <QObject>
#include <DependencyManager.h>

class GooglePolyScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    GooglePolyScriptingInterface();

public slots:
    void testPrint(QString input);
    void setAPIKey(QString key);

    QJsonArray getAssetList(QString keyword, QString category, QString format);
    QString getFBX(QString keyword, QString category);

private:
    QUrl formatURLQuery(QString keyword, QString category, QString format);
    QByteArray getHTTPRequest(QUrl url);
    QVariant parseJSON(QByteArray* response, int fileType);
    int getRandIntInRange(int length);
    //void onResult(QNetworkReply* reply);

    //QString authCode;
    
};

#endif // hifi_GooglePolyScriptingInterface_h