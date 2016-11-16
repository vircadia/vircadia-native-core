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

#include <Application.h>
#include <DependencyManager.h>
#include <GifCreator.h>
#include <qtimer.h>
#include "scripting/WindowScriptingInterface.h"

// If the snapshot width or the framerate are too high for the
// application to handle, the framerate of the output GIF will drop.
#define SNAPSNOT_ANIMATED_WIDTH (480)
// This value should divide evenly into 100. Snapshot framerate is NOT guaranteed.
#define SNAPSNOT_ANIMATED_TARGET_FRAMERATE (25)
#define SNAPSNOT_ANIMATED_DURATION_SECS (3)
#define SNAPSNOT_ANIMATED_DURATION_MSEC (SNAPSNOT_ANIMATED_DURATION_SECS*1000)

#define SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC (1000/SNAPSNOT_ANIMATED_TARGET_FRAMERATE)
// This is the fudge factor that we add to the *first* GIF frame's "delay" value
#define SNAPSNOT_ANIMATED_INITIAL_WRITE_DURATION_MSEC (20)
#define SNAPSNOT_ANIMATED_NUM_FRAMES (SNAPSNOT_ANIMATED_DURATION_SECS * SNAPSNOT_ANIMATED_TARGET_FRAMERATE)

class SnapshotAnimated {
private:
    static QTimer snapshotAnimatedTimer;
    static GifWriter snapshotAnimatedGifWriter;
    static qint64 snapshotAnimatedTimestamp;
    static qint64 snapshotAnimatedFirstFrameTimestamp;
    static qint64 snapshotAnimatedLastWriteFrameDuration;
    static bool snapshotAnimatedTimerRunning;
    static QString snapshotAnimatedPath;
    static QString snapshotStillPath;
public:
    static void saveSnapshotAnimated(QString pathStill, float aspectRatio, Application* app, QSharedPointer<WindowScriptingInterface> dm);
};

#endif // hifi_SnapshotAnimated_h
