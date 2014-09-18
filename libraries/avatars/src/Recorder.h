//
//  Recorder.h
//
//
//  Created by Clement on 8/7/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Recorder_h
#define hifi_Recorder_h

#include "Recording.h"

template<class C>
class QSharedPointer;

class AttachmentData;
class AvatarData;
class Recorder;
class Recording;

typedef QSharedPointer<Recorder> RecorderPointer;
typedef QWeakPointer<Recorder> WeakRecorderPointer;

/// Records a recording
class Recorder {
public:
    Recorder(AvatarData* avatar);
    
    bool isRecording() const;
    qint64 elapsed() const;
    
    RecordingPointer getRecording() const { return _recording; }
    
public slots:
    void startRecording();
    void stopRecording();
    void saveToFile(QString& file);
    void record();
    void record(char* samples, int size);
    
private:
    QElapsedTimer _timer;
    RecordingPointer _recording;

    AvatarData* _avatar;
};


#endif // hifi_Recorder_h