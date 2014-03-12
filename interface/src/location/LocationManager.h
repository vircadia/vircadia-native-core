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

    LocationManager();
    void createNamedLocation(NamedLocation* namedLocation);

    void goTo(QString destination);
    void goToUser(QString userName);
    void goToPlace(QString placeName);
    void goToOrientation(QString orientation);
    bool goToDestination(QString destination);

private:
    QString replaceLastOccurrence(QChar search, QChar replace, QString string);
    QJsonObject _userData;
    QJsonObject _placeData;

    void checkForMultipleDestinations();

signals:
    void creationCompleted(LocationManager::NamedLocationCreateResponse response, NamedLocation* location);
    void multipleDestinationsFound(const QJsonObject& userData, const QJsonObject& placeData);
    void locationChanged();
    
private slots:
    void namedLocationDataReceived(const QJsonObject& data);
    void errorDataReceived(QNetworkReply::NetworkError error, const QString& message);
    void goToLocationFromResponse(const QJsonObject& jsonObject);
    void goToUserFromResponse(const QJsonObject& jsonObject);

};

#endif /* defined(__hifi__LocationManager__) */
