//
//  ClientTraitsHandler.h
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 7/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ClientTraitsHandler_h
#define hifi_ClientTraitsHandler_h

#include <QtCore/QSharedPointer>

#include <ReceivedMessage.h>

#include "AssociatedTraitValues.h"
#include "Node.h"

class AvatarData;

class ClientTraitsHandler : public QObject {
    Q_OBJECT
public:
    ClientTraitsHandler(AvatarData* owningAvatar);

    int sendChangedTraitsToMixer();

    bool hasChangedTraits() const { return _hasChangedTraits; }

    void markTraitUpdated(AvatarTraits::TraitType updatedTrait);
    void markInstancedTraitUpdated(AvatarTraits::TraitType traitType, QUuid updatedInstanceID);
    void markInstancedTraitDeleted(AvatarTraits::TraitType traitType, QUuid deleteInstanceID);

    void resetForNewMixer();

public slots:
    void processTraitOverride(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

private:
    using Mutex = std::recursive_mutex;
    using Lock = std::lock_guard<Mutex>;

    enum ClientTraitStatus {
        Unchanged,
        Updated,
        Deleted
    };

    AvatarData* const _owningAvatar;

    Mutex _traitLock;
    AvatarTraits::AssociatedTraitValues<ClientTraitStatus, Unchanged> _traitStatuses;

    AvatarTraits::TraitVersion _currentTraitVersion { AvatarTraits::DEFAULT_TRAIT_VERSION };
    AvatarTraits::TraitVersion _currentSkeletonVersion { AvatarTraits::NULL_TRAIT_VERSION };
    
    bool _shouldPerformInitialSend { false };
    bool _hasChangedTraits { false };
};

#endif // hifi_ClientTraitsHandler_h
