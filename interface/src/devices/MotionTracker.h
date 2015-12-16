//
//  MotionTracker.h
//  interface/src/devices
//
//  Created by Sam Cake on 6/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MotionTracker_h
#define hifi_MotionTracker_h

#include "DeviceTracker.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#include <glm/glm.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif


#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

/// Base class for device trackers.
class MotionTracker : public DeviceTracker {
public:

    class Frame {
    public:
        Frame();

        glm::mat4 _transform;

        void setRotation(const glm::quat& rotation);
        void getRotation(glm::quat& rotatio) const;

        void setTranslation(const glm::vec3& translation);
        void getTranslation(glm::vec3& translation) const;
    };

    // Semantic and Index types to retreive the JointTrackers of this MotionTracker
    typedef std::string Semantic;
    typedef int32_t Index;
    static const Index INVALID_SEMANTIC = -1;
    static const Index INVALID_PARENT = -2;

    class JointTracker {
    public:
        typedef std::vector< JointTracker > Vector;
        typedef std::map< Semantic, Index > Map;

        JointTracker();
        JointTracker(const JointTracker& tracker);
        JointTracker(const Semantic& semantic, Index parent);

        const Frame& getLocFrame() const { return _locFrame; }
        Frame& editLocFrame() { return _locFrame; }
        void setLocFrame(const Frame& frame) { editLocFrame() = frame; }

        const Frame& getAbsFrame() const { return _absFrame; }
        Frame& editAbsFrame() { return _absFrame; }
        void setAbsFrame(const Frame& frame) { editAbsFrame() = frame; }

        const Semantic& getSemantic() const { return _semantic; }
        const Index& getParent() const { return _parent; }

        bool isActive() const { return (_lastUpdate <= 0); }
        void tickNewFrame() { _lastUpdate++; }
        void activeFrame() { _lastUpdate = 0; }

        /// Update the loc/abs transform for this joint from the current abs/loc value and the specified parent joint abs frame
        void updateLocFromAbsTransform(const JointTracker* parentJoint);
        void updateAbsFromLocTransform(const JointTracker* parentJoint);

    protected:
        Frame       _locFrame;
        Frame       _absFrame;
        Semantic    _semantic;
        Index       _parent;
        int         _lastUpdate;
    };

    virtual bool isConnected() const;

    Index numJointTrackers() const { return (Index)_jointsArray.size(); }

    /// Access a Joint from it's index.
    /// Index 0 is always the "Root".
    /// if the index is Invalid then returns NULL.
    const JointTracker* getJointTracker(Index index) const { return ((index > 0) && (index < (Index)(_jointsArray.size())) ? _jointsArray.data() + index : NULL); }
    JointTracker* editJointTracker(Index index) { return ((index > 0) && (index < (Index)(_jointsArray.size())) ? _jointsArray.data() + index : NULL); }

    /// From a semantic, find the Index of the Joint.
    /// \return the index of the mapped Joint or INVALID_SEMANTIC if the semantic is not knowned.
    Index findJointIndex(const Semantic& semantic) const;

protected:
    MotionTracker();
    virtual ~MotionTracker();

    JointTracker::Vector _jointsArray;
    JointTracker::Map _jointsMap;

    /// Adding joint is only done from the specialized Motion Tracker, hence this function is protected.
    /// The hierarchy of joints must be created from the top down to the branches.
    /// The "Root" node is at index 0 and exists at creation of the Motion Tracker.
    /// 
    /// \param semantic A joint is defined by it's semantic, the unique name mapping to it
    /// \param parent   The parent's index, the parent must be valid and correspond to a Joint that has been previously created
    ///
    /// \return The Index of the newly created Joint.
    ///         Valid if everything went well.
    ///         INVALID_SEMANTIC if the semantic is already in use
    ///         INVALID_PARENT if the parent is not valid
    Index addJoint(const Semantic& semantic, Index parent);

    /// Update the absolute transform stack traversing the hierarchy from the root down the branches
    /// This is a generic way to update all the Joint's absFrame by combining the locFrame going down the hierarchy branch.
    void updateAllAbsTransform();
};



#endif // hifi_MotionTracker_h
