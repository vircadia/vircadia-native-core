//
//  LocationManager.h
//  hifi
//
//  Created by Stojce Slavkovski on 2/7/14.
//
//

#ifndef __hifi__LocationManager__
#define __hifi__LocationManager__

#include <QtCore>
#include "NamedLocation.h"
#include "AccountManager.h"

class LocationManager : public QObject {
    Q_OBJECT

public:
    static LocationManager& getInstance();

    enum NamedLocationCreateResponse {
        Created,
        AlreadyExists,
        SystemError
    };

    LocationManager() { };
    void createNamedLocation(NamedLocation* namedLocation);

    void goTo(QString destination);
    void goToUser(QString userName);

signals:
    void creationCompleted(LocationManager::NamedLocationCreateResponse response, NamedLocation* location);
    
private slots:
    void namedLocationDataReceived(const QJsonObject& data);
    void errorDataReceived(QNetworkReply::NetworkError error, const QString& message);
};

#endif /* defined(__hifi__LocationManager__) */
