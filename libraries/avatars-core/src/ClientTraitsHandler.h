//
//  ClientTraitsHandler.h
//  libraries/avatars-core/src
//
//  Created by Nshan G. on 21 Apr 2022.
//  Copyright 2018 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_AVATARS_CORE_SRC_CLIENTTRAITSHANDLER_H
#define LIBRARIES_AVATARS_CORE_SRC_CLIENTTRAITSHANDLER_H

#include <QtCore/QSharedPointer>
#include <QtCore/QObject>

#include <ReceivedMessage.h>
#include <Node.h>

#include "AssociatedTraitValues.h"

template <typename Derived, typename AvatarPtr>
class ClientTraitsHandler {

public:
    ClientTraitsHandler(AvatarPtr owningAvatar);

    int sendChangedTraitsToMixer();

    bool hasChangedTraits() const { return _hasChangedTraits; }

    void markTraitUpdated(AvatarTraits::TraitType updatedTrait);
    void markInstancedTraitUpdated(AvatarTraits::TraitType traitType, QUuid updatedInstanceID);
    void markInstancedTraitDeleted(AvatarTraits::TraitType traitType, QUuid deleteInstanceID);

    void resetForNewMixer(SharedNodePointer mixerNode);

    void processTraitOverride(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

private:
    using Mutex = std::recursive_mutex;
    using Lock = std::lock_guard<Mutex>;

    enum ClientTraitStatus {
        Unchanged,
        Updated,
        Deleted
    };

    const AvatarPtr _owningAvatar;

    Mutex _traitLock;
    AvatarTraits::AssociatedTraitValues<ClientTraitStatus, Unchanged> _traitStatuses;

    AvatarTraits::TraitVersion _currentTraitVersion { AvatarTraits::DEFAULT_TRAIT_VERSION };
    AvatarTraits::TraitVersion _currentSkeletonVersion { AvatarTraits::NULL_TRAIT_VERSION };

    bool _shouldPerformInitialSend { false };
    bool _hasChangedTraits { false };

    Derived& derived();
    const Derived& derived() const;
};

#endif /* end of include guard */
