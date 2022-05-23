//
//  AvatarData.h
//  libraries/vircadia-client/src/internal/avatars
//
//  Created by Nshan G. on 9 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_AVATARDATA_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_AVATARDATA_H

#include <bitset>
#include <vector>
#include <algorithm>

#include <QObject>

#include <AvatarDataStream.h>

#include "../Common.h"
#include "../../avatars.h"
#include "ClientTraitsHandler.h"

namespace vircadia::client
{
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    inline bool operator==(const vircadia_vector& a, const vircadia_vector& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    inline bool operator!=(const vircadia_vector& a, const vircadia_vector& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_quaternion& a, const vircadia_quaternion& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }
    inline bool operator!=(const vircadia_quaternion& a, const vircadia_quaternion& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_bounds& a, const vircadia_bounds& b) {
        return a.dimensions == b.dimensions && a.offset == b.offset;
    }
    inline bool operator!=(const vircadia_bounds& a, const vircadia_bounds& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_vantage& a, const vircadia_vantage& b) {
        return a.position == b.position && a.rotation == b.rotation;
    }
    inline bool operator!=(const vircadia_vantage& a, const vircadia_vantage& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_transform& a, const vircadia_transform& b) {
        return a.vantage == b.vantage && a.scale == b.scale;
    }
    inline bool operator!=(const vircadia_transform& a, const vircadia_transform& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_avatar_additional_flags& a, const vircadia_avatar_additional_flags& b) {
        return a.hand_state == b.hand_state &&
            a.key_state == b.key_state &&
            bool(a.has_head_scripted_blendshapes) == bool(b.has_head_scripted_blendshapes) &&
            bool(a.has_procedural_eye_movement) == bool(b.has_procedural_eye_movement) &&
            bool(a.has_audio_face_movement) == bool(b.has_audio_face_movement) &&
            bool(a.has_procedural_eye_face_movement) == bool(b.has_procedural_eye_face_movement) &&
            bool(a.has_procedural_blink_face_movement) == bool(b.has_procedural_blink_face_movement) &&
            bool(a.collides_with_avatars) == bool(b.collides_with_avatars) &&
            bool(a.has_priority) == bool(b.has_priority);
    }
    inline bool operator!=(const vircadia_avatar_additional_flags& a, const vircadia_avatar_additional_flags& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_avatar_parent_info& a, const vircadia_avatar_parent_info& b) {
        return a.joint_index == b.joint_index &&
            std::equal(a.uuid, a.uuid+16, b.uuid);
    }
    inline bool operator!=(const vircadia_avatar_parent_info& a, const vircadia_avatar_parent_info& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_avatar_hand_controllers& a, const vircadia_avatar_hand_controllers& b) {
        return a.left == b.left && a.right == b.right;
    }
    inline bool operator!=(const vircadia_avatar_hand_controllers& a, const vircadia_avatar_hand_controllers& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_avatar_face_tracker_info& a, const vircadia_avatar_face_tracker_info& b) {
        assert(a.blendshape_coefficients_size < 256);
        assert(b.blendshape_coefficients_size < 256);
        return a.left_eye_blink == b.left_eye_blink &&
            a.right_eye_blink == b.right_eye_blink &&
            a.average_loundness == b.average_loundness &&
            a.brow_audio_lift == b.brow_audio_lift &&
            a.blendshape_coefficients_size == b.blendshape_coefficients_size &&
            std::equal(a.blendshape_coefficients, a.blendshape_coefficients + a.blendshape_coefficients_size, b.blendshape_coefficients);
    }
    inline bool operator!=(const vircadia_avatar_face_tracker_info& a, const vircadia_avatar_face_tracker_info& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_joint_flags& a, const vircadia_joint_flags& b) {
        return bool(a.translation_is_default) == bool(b.translation_is_default) &&
            bool(a.rotation_is_default) == bool(b.rotation_is_default);
    }
    inline bool operator!=(const vircadia_joint_flags& a, const vircadia_joint_flags& b) {
        return !(a == b);
    }

    inline bool operator==(const vircadia_far_grab_joints& a, const vircadia_far_grab_joints& b) {
        return a.left == b.left && a.right == b.right && a.mouse == b.mouse;
    }
    inline bool operator!=(const vircadia_far_grab_joints& a, const vircadia_far_grab_joints& b) {
        return !(a == b);
    }

    /// @private
    struct AvatarData {

        enum PropertyIndex : size_t {
            GlobalPositionIndex,
            BoundingBoxIndex,
            OrientationIndex,
            ScaleIndex,
            LookAtPositionIndex,
            AudioLoudnessIndex,
            SensorToWorldMatrixIndex,
            AdditionalFlagsIndex,
            ParentInfoIndex,
            LocalPositionIndex,
            HandControllersIndex,
            FaceTrackerInfoIndex,
            JointDataIndex,
            JointDefaultPoseFlagsIndex,
            GrabJointsIndex,

            IdentityIndex,
            SkeletonModelURLIndex,
        };

        QUuid sessionUUID;
        using Properties = std::tuple<
            vircadia_vector,                   // global position
            vircadia_bounds,                   // bounding box
            vircadia_quaternion,               // orientation
            float,                             // scale
            vircadia_vector,                   // look at position
            float,                             // audio loudness
            vircadia_transform,                // sensor to world matrix
            vircadia_avatar_additional_flags,  // additional flags
            vircadia_avatar_parent_info,       // parent info
            vircadia_vector,                   // local position
            vircadia_avatar_hand_controllers,  // hand controllers
            vircadia_avatar_face_tracker_info, // face tracker info
            std::vector<vircadia_vantage>,     // joint data
            std::vector<vircadia_joint_flags>, // joint default pose flags
            vircadia_far_grab_joints,          // grab joints

            AvatarDataPacket::Identity,        // identity
            std::string                        // skeleton model URL
        >;
        Properties properties;
        std::bitset<std::tuple_size_v<Properties>> changes;

        template <size_t Index, typename Accessor, typename Value =
            std::remove_reference_t<
                std::invoke_result_t<Accessor, Properties>>>
        bool setProperty(Accessor&& accessor, Value value) {
            const auto& current = accessor(std::get<Index>(properties));
            if (current != value) {
                accessor(std::get<Index>(properties), std::move(value));
                changes.set(Index);

                // NOTE: grab joint data is hard coded in packet
                // handling code to be sent and received together
                // with joint data only... not sure why though,
                // when something like joint default pose flags
                // are handled completely separately
                if (Index == AvatarData::GrabJointsIndex) {
                    changes.set(AvatarData::JointDataIndex);
                }

                return true;
            }
            return false;
        }

        template <PropertyIndex Index>
        bool setProperty(std::tuple_element_t<Index, Properties> value) {
            auto noopAccessor = overloaded {
                [](auto& x) -> decltype(auto) {
                    return (x);
                },
                [](auto& x, auto&& v){
                    x = std::forward<decltype(v)>(v);
                }
            };
            return setProperty<Index>(noopAccessor, value);
        }

        template <PropertyIndex Index, typename Value>
        bool setProperty(Value value, int element) {
            return setProperty<Index>( overloaded {
                [element](auto& x) -> decltype(auto) {
                    return (x[element]);
                },
                [element](auto& x, auto&& v){
                    x[element] = std::forward<decltype(v)>(v);
                },
            }, value);
        }

        template <PropertyIndex Index>
        bool resizeProperty(size_t size) {
            return setProperty<Index>( overloaded {
                [](auto&& x){
                    return x.size();
                },
                [](auto& x, size_t size){
                    x.resize(size);
                },
            }, size);
        }

        template <size_t Index>
        const auto& getProperty() const {
            return std::get<Index>(properties);
        }

    };

    /// @private
    class Avatar : public QObject, public AvatarDataStream<Avatar> {
        Q_OBJECT
    public:
        Avatar();

        const QUuid& getSessionUUID() const;
        void setSessionUUID(const QUuid& uuid);

        AvatarDataPacket::Identity getIdentityDataOut() const;
        bool getIdentityDataChanged();
        void onIdentityDataSent();

        // TODO: these in/out functions should be private

        AvatarDataPacket::AvatarGlobalPosition getGlobalPositionOut() const;
        void setGlobalPositionIn(const AvatarDataPacket::AvatarGlobalPosition&);

        AvatarDataPacket::AvatarBoundingBox getBoundingBoxOut() const;
        void setBoundingBoxIn(const AvatarDataPacket::AvatarBoundingBox&);

        glm::quat getOrientationOut() const;
        void setOrientationIn(glm::quat);

        float getScaleOut() const;
        void setScaleIn(float);

        AvatarDataPacket::LookAtPosition getLookAtPositionOut() const;
        void setLookAtPositionIn(const AvatarDataPacket::LookAtPosition&);

        float getAudioLoudnessOut() const;
        void setAudioLoudnessIn(float);

        struct Transform {
            glm::vec3 translation;
            glm::quat rotation;
            glm::vec3 scale;
        };

        Transform getSensorToWorldMatrixOut() const;
        void setSensorToWorldMatrixIn(Transform);

        using AdditionalFlags = std::tuple<
            KeyState, uint8_t, bool, bool, bool, bool, bool, bool, bool
        >;

        AdditionalFlags getAdditionalFlagsOut() const;
        void setAdditionalFlagsIn(vircadia_avatar_additional_flags);

        using ParentInfo = std::tuple<UUID, uint16_t>;

        ParentInfo getParentInfoOut() const;
        void setParentInfoIn(const uint8_t* uuid, uint16_t jointIndex);

        void onParseError(std::string message);
        void onPacketTooSmallError(std::string name, int sizeToRead, int sizeLeft);

        AvatarDataPacket::AvatarLocalPosition getLocalPositionOut() const;
        void setLocalPositionIn(const AvatarDataPacket::AvatarLocalPosition&);

        struct HandControllers {
            HandControllerVantage left;
            HandControllerVantage right;
        };

        HandControllers getHandControllersOut() const;
        void setHandControllersIn(const HandControllers&);

        using FaceTrackerInfo = std::tuple<float, float, float, float, uint8_t, const uint8_t*>;

        FaceTrackerInfo getFaceTrackerInfoOut() const;
        void setFaceTrackerInfoIn(const FaceTrackerInfo&);

        int getJointDataSizeOut() const;
        JointData getJointDataOut(int index) const;
        void setJointDataSizeIn(int size);
        void setJointDataPositionIn(int index, glm::vec3);
        void setJointDataRotationIn(int index, glm::quat);
        void setJointDataPositionDefaultIn(int index, bool);
        void setJointDataRotationDefaultIn(int index, bool);

        void setSkeletonModelURLIn(const QByteArray& encodedURL);

        AvatarData data;

    protected:
        ClientTraitsHandler* getClientTraitsHandler();
        const ClientTraitsHandler* getClientTraitsHandler() const;

        friend class AvatarDataStream<Avatar>;

    private:
        std::unique_ptr<ClientTraitsHandler, LaterDeleter> clientTraitsHandler;
    };

} // namespace vircadia::client

#endif /* end of include guard */
