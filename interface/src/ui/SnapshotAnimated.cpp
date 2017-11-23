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
#include <QtConcurrent/QtConcurrentRun>

#include <plugins/DisplayPlugin.h>
#include "SnapshotAnimated.h"

QTimer* SnapshotAnimated::snapshotAnimatedTimer = NULL;
qint64 SnapshotAnimated::snapshotAnimatedTimestamp = 0;
qint64 SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp = 0;
bool SnapshotAnimated::snapshotAnimatedTimerRunning = false;
QString SnapshotAnimated::snapshotAnimatedPath;
QString SnapshotAnimated::snapshotStillPath;
QVector<QImage> SnapshotAnimated::snapshotAnimatedFrameVector;
QVector<qint64> SnapshotAnimated::snapshotAnimatedFrameDelayVector;
Application* SnapshotAnimated::app;
float SnapshotAnimated::aspectRatio;
QSharedPointer<WindowScriptingInterface> SnapshotAnimated::snapshotAnimatedDM;
GifWriter SnapshotAnimated::snapshotAnimatedGifWriter;


Setting::Handle<bool> SnapshotAnimated::alsoTakeAnimatedSnapshot("alsoTakeAnimatedSnapshot", true);
Setting::Handle<float> SnapshotAnimated::snapshotAnimatedDuration("snapshotAnimatedDuration", SNAPSNOT_ANIMATED_DURATION_SECS);

void SnapshotAnimated::saveSnapshotAnimated(QString pathStill, float aspectRatio, Application* app, QSharedPointer<WindowScriptingInterface> dm) {
    // If we're not in the middle of capturing an animated snapshot...
    if (SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp == 0) {
        SnapshotAnimated::snapshotAnimatedTimer = new QTimer();
        SnapshotAnimated::aspectRatio = aspectRatio;
        SnapshotAnimated::app = app;
        SnapshotAnimated::snapshotAnimatedDM = dm;
        // Define the output location of the still and animated snapshots.
        SnapshotAnimated::snapshotStillPath = pathStill;
        SnapshotAnimated::snapshotAnimatedPath = pathStill;
        SnapshotAnimated::snapshotAnimatedPath.replace("jpg", "gif");

        // Ensure the snapshot timer is Precise (attempted millisecond precision)
        SnapshotAnimated::snapshotAnimatedTimer->setTimerType(Qt::PreciseTimer);

        // Connect the snapshotAnimatedTimer QTimer to the lambda slot function
        QObject::connect((SnapshotAnimated::snapshotAnimatedTimer), &QTimer::timeout, captureFrames);

        // Start the snapshotAnimatedTimer QTimer - argument for this is in milliseconds
        SnapshotAnimated::snapshotAnimatedTimerRunning = true;
        SnapshotAnimated::snapshotAnimatedTimer->start(SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC);
    }
}

void SnapshotAnimated::captureFrames() {
    if (SnapshotAnimated::snapshotAnimatedTimerRunning) {
        // Get a screenshot from the display, then scale the screenshot down,
        // then convert it to the image format the GIF library needs,
        // then save all that to the QImage named "frame"
        QImage frame(SnapshotAnimated::app->getActiveDisplayPlugin()->getScreenshot(SnapshotAnimated::aspectRatio));
        frame = frame.scaledToWidth(SNAPSNOT_ANIMATED_WIDTH);
        SnapshotAnimated::snapshotAnimatedFrameVector.append(frame);

        // If that was the first frame...
        if (SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp == 0) {
            // Record the current frame timestamp
            SnapshotAnimated::snapshotAnimatedTimestamp = QDateTime::currentMSecsSinceEpoch();
            // Record the first frame timestamp
            SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp = SnapshotAnimated::snapshotAnimatedTimestamp;
            SnapshotAnimated::snapshotAnimatedFrameDelayVector.append(SNAPSNOT_ANIMATED_FRAME_DELAY_MSEC / 10);
            // If this is an intermediate or the final frame...
        } else {
            // Push the current frame delay onto the vector
            SnapshotAnimated::snapshotAnimatedFrameDelayVector.append(round(((float)(QDateTime::currentMSecsSinceEpoch() - SnapshotAnimated::snapshotAnimatedTimestamp)) / 10));
            // Record the current frame timestamp
            SnapshotAnimated::snapshotAnimatedTimestamp = QDateTime::currentMSecsSinceEpoch();

            // If that was the last frame...
            if ((SnapshotAnimated::snapshotAnimatedTimestamp - SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp) >= (SnapshotAnimated::snapshotAnimatedDuration.get() * MSECS_PER_SECOND)) {
                SnapshotAnimated::snapshotAnimatedTimerRunning = false;

                // Notify the user that we're processing the snapshot
                // This also pops up the "Share" dialog. The unprocessed GIF will be visualized as a loading icon until processingGifCompleted() is called.
                emit SnapshotAnimated::snapshotAnimatedDM->processingGifStarted(SnapshotAnimated::snapshotStillPath);

                // Kick off the thread that'll pack the frames into the GIF
                QtConcurrent::run(processFrames);
                // Stop the snapshot QTimer. This action by itself DOES NOT GUARANTEE
                // that the slot will not be called again in the future.
                // See: http://lists.qt-project.org/pipermail/qt-interest-old/2009-October/013926.html
                SnapshotAnimated::snapshotAnimatedTimer->stop();
                delete SnapshotAnimated::snapshotAnimatedTimer;
            }
        }
    }
}

void SnapshotAnimated::processFrames() {
    uint32_t width = SnapshotAnimated::snapshotAnimatedFrameVector[0].width();
    uint32_t height = SnapshotAnimated::snapshotAnimatedFrameVector[0].height();

    // Create the GIF from the temporary files
    // Write out the header and beginning of the GIF file
    GifBegin(
        &(SnapshotAnimated::snapshotAnimatedGifWriter),
        qPrintable(SnapshotAnimated::snapshotAnimatedPath),
        width,
        height,
        1); // "1" means "yes there is a delay" with this GifCreator library.
    for (int itr = 0; itr < SnapshotAnimated::snapshotAnimatedFrameVector.size(); itr++) {
        // Write each frame to the GIF
        GifWriteFrame(&(SnapshotAnimated::snapshotAnimatedGifWriter),
            (uint8_t*)SnapshotAnimated::snapshotAnimatedFrameVector[itr].convertToFormat(QImage::Format_RGBA8888).bits(),
            width,
            height,
            SnapshotAnimated::snapshotAnimatedFrameDelayVector[itr]);
    }
    // Write out the end of the GIF
    GifEnd(&(SnapshotAnimated::snapshotAnimatedGifWriter));

    // Clear out the frame and frame delay vectors.
    // Also release the memory not required to store the items.
    SnapshotAnimated::snapshotAnimatedFrameVector.clear();
    SnapshotAnimated::snapshotAnimatedFrameVector.squeeze();
    SnapshotAnimated::snapshotAnimatedFrameDelayVector.clear();
    SnapshotAnimated::snapshotAnimatedFrameDelayVector.squeeze();
    // Reset the current frame timestamp
    SnapshotAnimated::snapshotAnimatedTimestamp = 0;
    SnapshotAnimated::snapshotAnimatedFirstFrameTimestamp = 0;
    
    // Update the "Share" dialog with the processed GIF.
    emit SnapshotAnimated::snapshotAnimatedDM->processingGifCompleted(SnapshotAnimated::snapshotAnimatedPath);
}
