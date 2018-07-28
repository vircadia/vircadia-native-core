//
//  Created by Bradley Austin Davis on 2015/11/13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RecordingScriptingInterface_h
#define hifi_RecordingScriptingInterface_h

#include <atomic>
#include <mutex>

#include <QtCore/QObject>

#include <BaseScriptEngine.h>
#include <DependencyManager.h>
#include <recording/ClipCache.h>
#include <recording/Forward.h>
#include <recording/Frame.h>

class QScriptEngine;
class QScriptValue;

/**jsdoc
 * @namespace Recording
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-assignment-client
 */
class RecordingScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    RecordingScriptingInterface();

public slots:

    /**jsdoc
     * @function Recording.loadRecording
     * @param {string} url
     * @param {Recording~loadRecordingCallback} [callback=null]
     */
    /**jsdoc
     * Called when {@link Recording.loadRecording} is complete.
     * @callback Recording~loadRecordingCallback
     * @param {boolean} success
     * @param {string} url
     */
    void loadRecording(const QString& url, QScriptValue callback = QScriptValue());


    /**jsdoc
     * @function Recording.startPlaying
     */
    void startPlaying();

    /**jsdoc
     * @function Recording.pausePlayer
     */
    void pausePlayer();

    /**jsdoc
     * @function Recording.stopPlaying
     */
    void stopPlaying();

    /**jsdoc
     * @function Recording.isPlaying
     * @returns {boolean}
     */
    bool isPlaying() const;

    /**jsdoc
     * @function Recording.isPaused
     * @returns {boolean}
     */
    bool isPaused() const;


    /**jsdoc
     * @function Recording.playerElapsed
     * @returns {number}
     */
    float playerElapsed() const;

    /**jsdoc
     * @function Recording.playerLength
     * @returns {number}
     */
    float playerLength() const;


    /**jsdoc
     * @function Recording.setPlayerVolume
     * @param {number} volume
     */
    void setPlayerVolume(float volume);

    /**jsdoc
     * @function Recording.setPlayerAudioOffset
     * @param {number} audioOffset
     */
    void setPlayerAudioOffset(float audioOffset);

    /**jsdoc
     * @function Recording.setPlayerTime
     * @param {number} time
     */
    void setPlayerTime(float time);

    /**jsdoc
     * @function Recording.setPlayerLoop
     * @param {boolean} loop
     */
    void setPlayerLoop(bool loop);


    /**jsdoc
     * @function Recording.setPlayerUseDisplayName
     * @param {boolean} useDisplayName
     */
    void setPlayerUseDisplayName(bool useDisplayName);

    /**jsdoc
     * @function Recording.setPlayerUseAttachments
     * @param {boolean} useAttachments
     */
    void setPlayerUseAttachments(bool useAttachments);

    /**jsdoc
     * @function Recording.setPlayerUseHeadModel
     * @param {boolean} useHeadModel
     * @todo <strong>Note:</strong> This function currently has no effect.
     */
    void setPlayerUseHeadModel(bool useHeadModel);

    /**jsdoc
     * @function Recording.setPlayerUseSkeletonModel
     * @param {boolean} useSkeletonModel
     * @todo <strong>Note:</strong> This function currently doesn't work.
     */
    void setPlayerUseSkeletonModel(bool useSkeletonModel);

    /**jsdoc
     * @function Recording.setPlayFromCurrentLocation
     * @param {boolean} playFromCurrentLocation
     */
    void setPlayFromCurrentLocation(bool playFromCurrentLocation);


    /**jsdoc
     * @function Recording.getPlayerUseDisplayName
     * @returns {boolean}
     */
    bool getPlayerUseDisplayName() { return _useDisplayName; }

    /**jsdoc
     * @function Recording.getPlayerUseAttachments
     * @returns {boolean}
     */
    bool getPlayerUseAttachments() { return _useAttachments; }

    /**jsdoc
     * @function Recording.getPlayerUseHeadModel
     * @returns {boolean}
     */
    bool getPlayerUseHeadModel() { return _useHeadModel; }

    /**jsdoc
     * @function Recording.getPlayerUseSkeletonModel
     * @returns {boolean}
     */
    bool getPlayerUseSkeletonModel() { return _useSkeletonModel; }

    /**jsdoc
     * @function Recording.getPlayFromCurrentLocation
     * @returns {boolean}
     */
    bool getPlayFromCurrentLocation() { return _playFromCurrentLocation; }


    /**jsdoc
     * @function Recording.startRecording
     */
    void startRecording();

    /**jsdoc
     * @function Recording.stopRecording
     */
    void stopRecording();

    /**jsdoc
     * @function Recording.isRecording
     * @returns {boolean}
     */
    bool isRecording() const;


    /**jsdoc
     * @function Recording.recorderElapsed
     * @returns {number}
     */
    float recorderElapsed() const;


    /**jsdoc
     * @function Recording.getDefaultRecordingSaveDirectory
     * @returns {string}
     */
    QString getDefaultRecordingSaveDirectory();

    /**jsdoc
     * @function Recording.saveRecording
     * @param {string} filename
     */
    void saveRecording(const QString& filename);

    /**jsdoc
     * @function Recording.saveRecordingToAsset
     * @param {function} getClipAtpUrl
     */
    bool saveRecordingToAsset(QScriptValue getClipAtpUrl);

    /**jsdoc
     * @function Recording.loadLastRecording
     */
    void loadLastRecording();

protected:
    using Mutex = std::recursive_mutex;
    using Locker = std::unique_lock<Mutex>;
    using Flag = std::atomic<bool>;

    QSharedPointer<recording::Deck> _player;
    QSharedPointer<recording::Recorder> _recorder;
    
    Flag _playFromCurrentLocation { true };
    Flag _useDisplayName { false };
    Flag _useHeadModel { false };
    Flag _useAttachments { false };
    Flag _useSkeletonModel { false };
    recording::ClipPointer _lastClip;

    QSet<recording::NetworkClipLoaderPointer> _clipLoaders;

private:
    void playClip(recording::NetworkClipLoaderPointer clipLoader, const QString& url, QScriptValue callback);
};

#endif // hifi_RecordingScriptingInterface_h
