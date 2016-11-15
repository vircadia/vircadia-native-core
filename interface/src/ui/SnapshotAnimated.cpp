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

void SnapshotAnimated::saveSnapshotAnimated(bool includeAnimated, QString pathStillSnapshot, float aspectRatio, Application* app, QSharedPointer<WindowScriptingInterface> dm) {
    // If we're not in the middle of capturing an animated snapshot...
    if ((snapshotAnimatedFirstFrameTimestamp == 0) && (includeAnimated))
    {
        // Define the output location of the animated snapshot
        QString pathAnimatedSnapshot(pathStillSnapshot);
        pathAnimatedSnapshot.replace("jpg", "gif");
        // Reset the current animated snapshot last frame duration
        snapshotAnimatedLastWriteFrameDuration = SNAPSNOT_ANIMATED_INITIAL_WRITE_DURATION_MSEC;

        // Ensure the snapshot timer is Precise (attempted millisecond precision)
        snapshotAnimatedTimer.setTimerType(Qt::PreciseTimer);

        // Connect the snapshotAnimatedTimer QTimer to the lambda slot function
        QObject::connect(&(snapshotAnimatedTimer), &QTimer::timeout, [=] {
            // Get a screenshot from the display, then scale the screenshot down,
            // then convert it to the image format the GIF library needs,
            // then save all that to the QImage named "frame"
            QImage frame(app->getActiveDisplayPlugin()->getScreenshot(aspectRatio));
            frame = frame.scaledToWidth(SNAPSNOT_ANIMATED_WIDTH);
            frame = frame.convertToFormat(QImage::Format_RGBA8888);

            // If this is the first frame...
            if (snapshotAnimatedTimestamp == 0)
            {
                // Write out the header and beginning of the GIF file
                GifBegin(&(snapshotAnimatedGifWriter), qPrintable(pathAnimatedSnapshot), frame.width(), frame.height(), SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC / 10);
                // Write the first to the gif
                GifWriteFrame(&(snapshotAnimatedGifWriter),
                    (uint8_t*)frame.bits(),
                    frame.width(),
                    frame.height(),
                    SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC / 10);
                // Record the current frame timestamp
                snapshotAnimatedTimestamp = QDateTime::currentMSecsSinceEpoch();
                snapshotAnimatedFirstFrameTimestamp = snapshotAnimatedTimestamp;
            }
            else
            {
                // If that was the last frame...
                if ((snapshotAnimatedTimestamp - snapshotAnimatedFirstFrameTimestamp) >= (SNAPSNOT_ANIMATED_DURATION_SECS * 1000))
                {
                    // Reset the current frame timestamp
                    snapshotAnimatedTimestamp = 0;
                    snapshotAnimatedFirstFrameTimestamp = 0;
                    // Write out the end of the GIF
                    GifEnd(&(snapshotAnimatedGifWriter));
                    // Stop the snapshot QTimer
                    snapshotAnimatedTimer.stop();
                    emit dm->snapshotTaken(pathStillSnapshot, pathAnimatedSnapshot, false);
                    qDebug() << "still: " << pathStillSnapshot << "anim: " << pathAnimatedSnapshot;
                    //emit dm->snapshotTaken("C:\\Users\\Zach Fox\\Desktop\\hifi-snap-by-zfox-on-2016-11-14_17-07-33.jpg", "C:\\Users\\Zach Fox\\Desktop\\hifi-snap-by-zfox-on-2016-11-14_17-10-02.gif", false);
                }
                else
                {
                    // Variable used to determine how long the current frame took to pack
                    qint64 framePackStartTime = QDateTime::currentMSecsSinceEpoch();
                    // Write the frame to the gif
                    GifWriteFrame(&(snapshotAnimatedGifWriter),
                        (uint8_t*)frame.bits(),
                        frame.width(),
                        frame.height(),
                        round(((float)(QDateTime::currentMSecsSinceEpoch() - snapshotAnimatedTimestamp + snapshotAnimatedLastWriteFrameDuration)) / 10));
                    // Record how long it took for the current frame to pack
                    snapshotAnimatedLastWriteFrameDuration = QDateTime::currentMSecsSinceEpoch() - framePackStartTime;
                    // Record the current frame timestamp
                    snapshotAnimatedTimestamp = QDateTime::currentMSecsSinceEpoch();
                }
            }
        });

        // Start the snapshotAnimatedTimer QTimer - argument for this is in milliseconds
        snapshotAnimatedTimer.start(SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC);
    }
    // If we're already in the middle of capturing an animated snapshot...
    else
    {
        // Just tell the dependency manager that the capture of the still snapshot has taken place.
        emit dm->snapshotTaken(pathStillSnapshot, "", false);
    }
}
