//
//  MixerAvatar.h
//  assignment-client/src/avatars
//
//  Created by Simon Walton Feb 2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Avatar class for use within the avatar mixer - encapsulates data required only for
// sorting priorities within the mixer.

#ifndef hifi_MixerAvatar_h
#define hifi_MixerAvatar_h

#include <AvatarData.h>

class ResourceRequest;

class MixerAvatar : public AvatarData {
public:
    bool getNeedsHeroCheck() const { return _needsHeroCheck; }
    void setNeedsHeroCheck(bool needsHeroCheck = true) { _needsHeroCheck = needsHeroCheck; }

    void fetchAvatarFST();

private:
    bool _needsHeroCheck{ false };

    // Avatar certification/verification
    enum VerifyState { kNoncertified, kRequestingFST, kReceivedFST, kStaticValidation, kRequestingOwner, kChallengeClient, kVerified,
        kVerificationFailed, kVerificationSucceeded, kError };
    Q_ENUM(VerifyState);
    VerifyState _verifyState { kNoncertified };
    QMutex _avatarCertifyLock;
    ResourceRequest* _avatarRequest { nullptr };
    QString _marketplaceIdString;
    QByteArray _avatarFSTContents;
    QByteArray _certificateHash;
    QString _certificateIdFromURL;
    QString _certificateIdFromFST;
    bool _avatarFSTValid { false };

    bool generateFSTHash();
    bool validateFSTHash(const QString& publicKey);
    QByteArray canonicalJson(const QString fstFile);

private slots:
    void fstRequestComplete();
};

using MixerAvatarSharedPointer = std::shared_ptr<MixerAvatar>;

#endif  // hifi_MixerAvatar_h
