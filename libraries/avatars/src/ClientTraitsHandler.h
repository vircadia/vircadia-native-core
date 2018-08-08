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

#include <ReceivedMessage.h>

#include "AvatarTraits.h"
#include "Node.h"

class AvatarData;

class ClientTraitsHandler : public QObject {
    Q_OBJECT
public:
    ClientTraitsHandler(AvatarData* owningAvatar);

    void sendChangedTraitsToMixer();

    bool hasChangedTraits() { return _changedTraits.hasAny(); }
    void markTraitChanged(AvatarTraits::TraitType changedTrait) { _changedTraits.insert(changedTrait); }

    bool hasTraitChanged(AvatarTraits::TraitType checkTrait) { return _changedTraits.contains(checkTrait) > 0; }

    void resetForNewMixer();

public slots:
    void processTraitOverride(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

private:
    AvatarData* _owningAvatar;

    AvatarTraits::TraitTypeSet _changedTraits;
    AvatarTraits::TraitVersion _currentTraitVersion { AvatarTraits::DEFAULT_TRAIT_VERSION };

    AvatarTraits::NullableTraitVersion _currentSkeletonVersion { AvatarTraits::NULL_TRAIT_VERSION };
    
    bool _performInitialSend { false };
};

#endif // hifi_ClientTraitsHandler_h
