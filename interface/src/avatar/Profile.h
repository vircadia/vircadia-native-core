//
//  Profile.h
//  hifi
//
//  Created by Stephen Birarda on 10/8/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Profile__
#define __hifi__Profile__

#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QUuid>

class Profile {
public:
    Profile();
    
    void setUsername(const QString& username);
    QString& getUsername() { return _username; }
    
    void setUUID(const QUuid& uuid);
    QUuid& getUUID() { return _uuid; }
    
    void setFaceModelURL(const QUrl& faceModelURL) { _faceModelURL = faceModelURL; }
    QUrl& getFaceModelURL() { return _faceModelURL; }
    
    void clear();
    
    void saveData(QSettings* settings);
    void loadData(QSettings* settings);
private:
    QString _username;
    QUuid _uuid;
    QUrl _faceModelURL;
};

#endif /* defined(__hifi__Profile__) */
