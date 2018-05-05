//
//  DiffTraversal.h
//  assignment-client/src/entities
//
//  Created by Andrew Meadows 2017.08.08
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DiffTraversal_h
#define hifi_DiffTraversal_h

#include <ViewFrustum.h>

#include "EntityTreeElement.h"

// DiffTraversal traverses the tree and applies _scanElementCallback on elements it finds
class DiffTraversal {
public:
    // VisibleElement is a struct identifying an element and how it intersected the view.
    // The intersection is used to optimize culling entities from the sendQueue.
    class VisibleElement {
    public:
        EntityTreeElementPointer element;
        ViewFrustum::intersection intersection { ViewFrustum::OUTSIDE };
    };

    // View is a struct with a ViewFrustum and LOD parameters
    class View {
    public:
        bool isBigEnough(const AACube& cube, float minDiameter = MIN_ENTITY_ANGULAR_DIAMETER) const;
        bool intersects(const AACube& cube) const;
        bool usesViewFrustums() const;
        bool isVerySimilar(const View& view) const;
        ViewFrustum::intersection calculateIntersection(const AACube& cube) const;

        ViewFrustums viewFrustums;
        uint64_t startTime { 0 };
        float lodScaleFactor { 1.0f };
    };

    // Waypoint is an bookmark in a "path" of waypoints during a traversal.
    class Waypoint {
    public:
        Waypoint(EntityTreeElementPointer& element);

        void getNextVisibleElementFirstTime(VisibleElement& next, const View& view);
        void getNextVisibleElementRepeat(VisibleElement& next, const View& view, uint64_t lastTime);
        void getNextVisibleElementDifferential(VisibleElement& next, const View& view, const View& lastView);

        int8_t getNextIndex() const { return _nextIndex; }
        void initRootNextIndex() { _nextIndex = -1; }

    protected:
        EntityTreeElementWeakPointer _weakElement;
        int8_t _nextIndex;
    };

    typedef enum { First, Repeat, Differential } Type;

    DiffTraversal();

    Type prepareNewTraversal(const DiffTraversal::View& view, EntityTreeElementPointer root);

    const View& getCurrentView() const { return _currentView; }
    const View& getCompletedView() const { return _completedView; }

    bool doesCurrentUseViewFrustum() const { return _currentView.usesViewFrustums(); }

    uint64_t getStartOfCompletedTraversal() const { return _completedView.startTime; }
    bool finished() const { return _path.empty(); }

    void setScanCallback(std::function<void (VisibleElement&)> cb);
    void traverse(uint64_t timeBudget);

    void reset() { _path.clear(); _completedView.startTime = 0; } // resets our state to force a new "First" traversal

private:
    void getNextVisibleElement(VisibleElement& next);

    View _currentView;
    View _completedView;
    std::vector<Waypoint> _path;
    std::function<void (VisibleElement&)> _getNextVisibleElementCallback { nullptr };
    std::function<void (VisibleElement&)> _scanElementCallback { [](VisibleElement& e){} };
};

#endif // hifi_EntityPriorityQueue_h
