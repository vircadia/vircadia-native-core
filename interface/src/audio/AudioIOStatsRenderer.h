//
//  AudioIOStatsRenderer.h
//  interface/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioIOStatsRenderer_h
#define hifi_AudioIOStatsRenderer_h

#include <QObject>

#include <DependencyManager.h>

class AudioIOStats;
class AudioStreamStats;

class AudioIOStatsRenderer : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:
    void render(const float* color, int width, int height);
    
public slots:
    void toggle() { _isEnabled = !_isEnabled; }
    void toggleShowInjectedStreams() { _shouldShowInjectedStreams = !_shouldShowInjectedStreams; }
protected:
    AudioIOStatsRenderer();
private:
    // audio stats methods for rendering
    void renderAudioStreamStats(const AudioStreamStats* streamStats, int horizontalOffset, int& verticalOffset,
                                float scale, float rotation, int font, const float* color, bool isDownstreamStats = false);
    
    const AudioIOStats* _stats;
    
    bool _isEnabled;
    bool _shouldShowInjectedStreams;
};


#endif // hifi_AudioIOStatsRenderer_h