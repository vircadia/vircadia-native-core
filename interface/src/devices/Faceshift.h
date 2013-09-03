//
//  Faceshift.h
//  interface
//
//  Created by Andrzej Kapolka on 9/3/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Faceshift__
#define __interface__Faceshift__

#include <QTcpSocket>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <fsbinarystream.h>

/// Handles interaction with the Faceshift software, which provides head position/orientation and facial features.
class Faceshift : public QObject {
    Q_OBJECT

public:

    Faceshift();

    bool isActive() const { return _socket.state() == QAbstractSocket::ConnectedState; }

    const glm::quat& getHeadRotation() const { return _headRotation; }
    const glm::vec3& getHeadTranslation() const { return _headTranslation; }

public slots:
    
    void setEnabled(bool enabled);

private slots:

    void connectSocket();
    void readFromSocket();        
    
private:
    
    QTcpSocket _socket;
    fs::fsBinaryStream _stream;
    bool _enabled;
    
    glm::quat _headRotation;
    glm::vec3 _headTranslation;
};

#endif /* defined(__interface__Faceshift__) */
