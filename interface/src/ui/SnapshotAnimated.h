//
//  SnapshotAnimated.h
//  interface/src/ui
//
//  Created by Zach Fox on 11/14/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SnapshotAnimated_h
#define hifi_SnapshotAnimated_h

#include <QtCore/QSharedPointer>
#include <QtCore/QVector>
#include <Application.h>
#include <DependencyManager.h>
#include <GifCreator.h>
#include <qtimer.h>
#include <SettingHandle.h>
#include "scripting/WindowScriptingInterface.h"

// If the snapshot width or the framerate are too high for the
// computer to handle, the framerate of the output GIF will drop.
#define SNAPSNOT_ANIMATED_WIDTH (720)
// This value should divide evenly into 100. Snapshot framerate is NOT guaranteed.
#define SNAPSNOT_ANIMATED_TARGET_FRAMERATE (25)
#define SNAPSNOT_ANIMATED_DURATION_SECS (3)
#define SNAPSNOT_ANIMATED_DURATION_MSEC (SNAPSNOT_ANIMATED_DURATION_SECS*1000)

#define SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC (1000/SNAPSNOT_ANIMATED_TARGET_FRAMERATE)

class SnapshotAnimated {
private:
    static QTimer* snapshotAnimatedTimer;
    static qint64 snapshotAnimatedTimestamp;
    static qint64 snapshotAnimatedFirstFrameTimestamp;
    static bool snapshotAnimatedTimerRunning;
    static QString snapshotStillPath;

    static QString snapshotAnimatedPath;
    static QVector<QImage> snapshotAnimatedFrameVector;
    static QVector<qint64> snapshotAnimatedFrameDelayVector;
    static QSharedPointer<WindowScriptingInterface> snapshotAnimatedDM;
    static float aspectRatio;

    static GifWriter snapshotAnimatedGifWriter;

    static void captureFrames();
    static void processFrames();
    static void clearTempVariables();
public:
    static void saveSnapshotAnimated(QString pathStill, float aspectRatio, QSharedPointer<WindowScriptingInterface> dm);
    static bool isAlreadyTakingSnapshotAnimated() { return snapshotAnimatedFirstFrameTimestamp != 0; };
    static Setting::Handle<bool> alsoTakeAnimatedSnapshot;
    static Setting::Handle<float> snapshotAnimatedDuration;
};

#endif // hifi_SnapshotAnimated_h
