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
    void testPrint();
    void setAPIKey(QString key);

    //QJsonArray GooglePolyScriptingInterface::getAssetList();
    void getAssetList();

private:
    QByteArray getHTTPRequest(QUrl url);
    QJsonObject makeJSONObject(QByteArray* response, bool isList);
    //void onResult(QNetworkReply* reply);

    QString authCode;
    

};

#endif // hifi_GooglePolyScriptingInterface_h