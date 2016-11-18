//
//  SnapshotAnimated.cpp
//  interface/src/ui
//
//  Created by Zach Fox on 11/14/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QImage>

#include "SnapshotAnimated.h"

QTimer SnapshotAnimated::snapshotAnimatedTimer;
GifWriter SnapshotAnimated::snapshotAnimatedGifWriter;
qint64 SnapshotAnimated::snapshotAnimatedTimestamp = 0;
qint64 SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp = 0;
qint64 SnapshotAnimated::snapshotAnimatedLastWriteFrameDuration = 0;
bool SnapshotAnimated::snapshotAnimatedTimerRunning = false;
QString SnapshotAnimated::snapshotAnimatedPath;
QString SnapshotAnimated::snapshotStillPath;

Setting::Handle<bool> SnapshotAnimated::alsoTakeAnimatedSnapshot("alsoTakeAnimatedSnapshot", true);
Setting::Handle<float> SnapshotAnimated::snapshotAnimatedDuration("snapshotAnimatedDuration", SNAPSNOT_ANIMATED_DURATION_SECS);

void SnapshotAnimated::saveSnapshotAnimated(QString pathStill, float aspectRatio, Application* app, QSharedPointer<WindowScriptingInterface> dm) {
    // If we're not in the middle of capturing an animated snapshot...
    if (SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp == 0) {
        // Define the output location of the still and animated snapshots.
        SnapshotAnimated::snapshotStillPath = pathStill;
        SnapshotAnimated::snapshotAnimatedPath = pathStill;
        SnapshotAnimated::snapshotAnimatedPath.replace("jpg", "gif");
        // Reset the current animated snapshot last frame duration
        SnapshotAnimated::snapshotAnimatedLastWriteFrameDuration = SNAPSNOT_ANIMATED_INITIAL_WRITE_DURATION_MSEC;

        // Ensure the snapshot timer is Precise (attempted millisecond precision)
        SnapshotAnimated::snapshotAnimatedTimer.setTimerType(Qt::PreciseTimer);

        // Connect the snapshotAnimatedTimer QTimer to the lambda slot function
        QObject::connect(&(SnapshotAnimated::snapshotAnimatedTimer), &QTimer::timeout, [=] {
            if (SnapshotAnimated::snapshotAnimatedTimerRunning) {
                // Get a screenshot from the display, then scale the screenshot down,
                // then convert it to the image format the GIF library needs,
                // then save all that to the QImage named "frame"
                QImage frame(app->getActiveDisplayPlugin()->getScreenshot(aspectRatio));
                frame = frame.scaledToWidth(SNAPSNOT_ANIMATED_WIDTH).convertToFormat(QImage::Format_RGBA8888);

                // If this is an intermediate or the final frame...
                if (SnapshotAnimated::snapshotAnimatedTimestamp > 0) {
                    // Variable used to determine how long the current frame took to pack
                    qint64 framePackStartTime = QDateTime::currentMSecsSinceEpoch();
                    // Write the frame to the gif
                    GifWriteFrame(&(SnapshotAnimated::snapshotAnimatedGifWriter),
                        (uint8_t*)frame.bits(),
                        frame.width(),
                        frame.height(),
                        round(((float)(framePackStartTime - SnapshotAnimated::snapshotAnimatedTimestamp + SnapshotAnimated::snapshotAnimatedLastWriteFrameDuration)) / 10));
                    // Record the current frame timestamp
                    SnapshotAnimated::snapshotAnimatedTimestamp = QDateTime::currentMSecsSinceEpoch();
                    // Record how long it took for the current frame to pack
                    SnapshotAnimated::snapshotAnimatedLastWriteFrameDuration = SnapshotAnimated::snapshotAnimatedTimestamp - framePackStartTime;
                    // If that was the last frame...
                    if ((SnapshotAnimated::snapshotAnimatedTimestamp - SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp) >= (SnapshotAnimated::snapshotAnimatedDuration.get() * MSECS_PER_SECOND)) {
                        // Stop the snapshot QTimer. This action by itself DOES NOT GUARANTEE
                        // that the slot will not be called again in the future.
                        // See: http://lists.qt-project.org/pipermail/qt-interest-old/2009-October/013926.html
                        SnapshotAnimated::snapshotAnimatedTimer.stop();
                        SnapshotAnimated::snapshotAnimatedTimerRunning = false;
                        // Reset the current frame timestamp
                        SnapshotAnimated::snapshotAnimatedTimestamp = 0;
                        SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp = 0;
                        // Write out the end of the GIF
                        GifEnd(&(SnapshotAnimated::snapshotAnimatedGifWriter));
                        // Let the dependency manager know that the snapshots have been taken.
                        emit dm->snapshotTaken(SnapshotAnimated::snapshotStillPath, SnapshotAnimated::snapshotAnimatedPath, false);
                    }
                // If that was the first frame...
                } else {
                    // Write out the header and beginning of the GIF file
                    GifBegin(&(SnapshotAnimated::snapshotAnimatedGifWriter), qPrintable(SnapshotAnimated::snapshotAnimatedPath), frame.width(), frame.height(), SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC / 10);
                    // Write the first to the gif
                    GifWriteFrame(&(SnapshotAnimated::snapshotAnimatedGifWriter),
                        (uint8_t*)frame.bits(),
                        frame.width(),
                        frame.height(),
                        SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC / 10);
                    // Record the current frame timestamp
                    SnapshotAnimated::snapshotAnimatedTimestamp = QDateTime::currentMSecsSinceEpoch();
                    SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp = SnapshotAnimated::snapshotAnimatedTimestamp;
                }
            }
        });

        // Start the snapshotAnimatedTimer QTimer - argument for this is in milliseconds
        SnapshotAnimated::snapshotAnimatedTimer.start(SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC);
        SnapshotAnimated::snapshotAnimatedTimerRunning = true;
    // If we're already in the middle of capturing an animated snapshot...
    } else {
        // Just tell the dependency manager that the capture of the still snapshot has taken place.
        emit dm->snapshotTaken(pathStill, "", false);
    }
}
