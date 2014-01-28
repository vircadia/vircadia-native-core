//
//  Snapshot.h
//  hifi
//
//  Created by Stojce Slavkovski on 1/26/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Snapshot__
#define __hifi__Snapshot__

#include <QString>
#include <QImage>
#include <QGLWidget>

#include <glm/glm.hpp>

class Snapshot {

public:
    static void saveSnapshot(QGLWidget* widget, QString username, glm::vec3 location);

private:
    QString _username;
};

#endif /* defined(__hifi__Snapshot__) */
