//
//  CameraToolBox.h
//  interface/src/devices
//
//  Created by David Rowe on 30 Apr 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CameraToolBox_h
#define hifi_CameraToolBox_h

#include <QObject>

#include <DependencyManager.h>
#include <GeometryCache.h>

class CameraToolBox : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    void render(int x, int y, bool boxed);
    bool mousePressEvent(int x, int y);
    bool mouseDoublePressEvent(int x, int y);

protected:
    CameraToolBox();
    ~CameraToolBox();

private slots:
    void toggleMute();

private:
    gpu::TexturePointer _enabledTexture;
    gpu::TexturePointer _mutedTexture;
    int _boxQuadID = GeometryCache::UNKNOWN_ID;
    QRect _iconBounds;
    qint64 _iconPulseTimeReference = 0;
    QTimer* _doubleClickTimer;
};

#endif // hifi_CameraToolBox_h