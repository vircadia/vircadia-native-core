//
//  UserLocationsModel.h
//  interface/src
//
//  Created by Ryan Huffman on 06/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UserLocationsModel_h
#define hifi_UserLocationsModel_h

#include <QAbstractListModel>
#include <QModelIndex>
#include <QVariant>


class UserLocation : public QObject {
    Q_OBJECT
public:
    UserLocation(const QString& id, const QString& name, const QString& address);
    bool isUpdating() { return _updating; }
    void requestRename(const QString& newName);
    void requestDelete();

    const QString& id() { return _id; }
    const QString& name() { return _name; }
    const QString& address() { return _address; }

public slots:
    void handleRenameResponse(const QJsonObject& responseData);
    void handleRenameError(QNetworkReply& errorReply);
    void handleDeleteResponse(const QJsonObject& responseData);
    void handleDeleteError(QNetworkReply& errorReply);

signals:
    void updated(const QString& name);
    void deleted(const QString& name);

private:
    QString _id;
    QString _name;
    QString _address;
    QString _previousName;
    bool _updating;

};

class UserLocationsModel : public QAbstractListModel {
    Q_OBJECT
public:
    UserLocationsModel(QObject* parent = NULL);
    ~UserLocationsModel();
    
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const { return 2; };
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;

    void deleteLocation(const QModelIndex& index);
    void renameLocation(const QModelIndex& index, const QString& newName);

    enum Columns {
        NameColumn = 0,
        AddressColumn
    };

public slots:
    void refresh();
    void update();
    void handleLocationsResponse(const QJsonObject& responseData);
    void removeLocation(const QString& name);

private:
    bool _updating;
    QList<UserLocation*> _locations;
};

#endif // hifi_UserLocationsModel_h
