//
//  DiffTraversal.cpp
//  assignment-client/src/entities
//
//  Created by Andrew Meadows 2017.08.08
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DiffTraversal.h"

#include <OctreeUtils.h>


DiffTraversal::Waypoint::Waypoint(EntityTreeElementPointer& element) : _nextIndex(0) {
    assert(element);
    _weakElement = element;
}

void DiffTraversal::Waypoint::getNextVisibleElementFirstTime(DiffTraversal::VisibleElement& next,
        const DiffTraversal::View& view) {
    // NOTE: no need to set next.intersection in the "FirstTime" context
    if (_nextIndex == -1) {
        // root case is special:
        // its intersection is always INTERSECT,
        // we never bother checking for LOD culling, and
        // we can skip it if the content hasn't changed
        ++_nextIndex;
        next.element = _weakElement.lock();
        return;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            // check for LOD truncation
            float visibleLimit = boundaryDistanceForRenderLevel(element->getLevel() + view.lodLevelOffset, view.rootSizeScale);
            float distance2 = glm::distance2(view.viewFrustum.getPosition(), element->getAACube().calcCenter());
            if (distance2 < visibleLimit * visibleLimit) {
                while (_nextIndex < NUMBER_OF_CHILDREN) {
                    EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                    ++_nextIndex;
                    if (nextElement && view.viewFrustum.cubeIntersectsKeyhole(nextElement->getAACube())) {
                        next.element = nextElement;
                        return;
                    }
                }
            }
        }
    }
    next.element.reset();
}

void DiffTraversal::Waypoint::getNextVisibleElementRepeat(
        DiffTraversal::VisibleElement& next, const DiffTraversal::View& view, uint64_t lastTime) {
    if (_nextIndex == -1) {
        // root case is special
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        if (element->getLastChangedContent() > lastTime) {
            next.element = element;
            next.intersection = ViewFrustum::INTERSECT;
            return;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            // check for LOD truncation
            float visibleLimit = boundaryDistanceForRenderLevel(element->getLevel() + view.lodLevelOffset, view.rootSizeScale);
            float distance2 = glm::distance2(view.viewFrustum.getPosition(), element->getAACube().calcCenter());
            if (distance2 < visibleLimit * visibleLimit) {
                while (_nextIndex < NUMBER_OF_CHILDREN) {
                    EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                    ++_nextIndex;
                    if (nextElement && nextElement->getLastChanged() > lastTime) {
                        ViewFrustum::intersection intersection = view.viewFrustum.calculateCubeKeyholeIntersection(nextElement->getAACube());
                        if (intersection != ViewFrustum::OUTSIDE) {
                            next.element = nextElement;
                            next.intersection = intersection;
                            return;
                        }
                    }
                }
            }
        }
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}

DiffTraversal::DiffTraversal() {
    const int32_t MIN_PATH_DEPTH = 16;
    _path.reserve(MIN_PATH_DEPTH);
}

void DiffTraversal::Waypoint::getNextVisibleElementDifferential(DiffTraversal::VisibleElement& next,
        const DiffTraversal::View& view, const DiffTraversal::View& lastView) {
    if (_nextIndex == -1) {
        // root case is special
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        if (element->getLastChangedContent() > lastView.startTime) {
            next.element = element;
            next.intersection = ViewFrustum::INTERSECT;
            return;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            // check for LOD truncation
            int32_t level = element->getLevel() + view.lodLevelOffset;
            float visibleLimit = boundaryDistanceForRenderLevel(level, view.rootSizeScale);
            float distance2 = glm::distance2(view.viewFrustum.getPosition(), element->getAACube().calcCenter());
            if (distance2 < visibleLimit * visibleLimit) {
                while (_nextIndex < NUMBER_OF_CHILDREN) {
                    EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                    ++_nextIndex;
                    if (nextElement) {
                        AACube cube = nextElement->getAACube();
                        if ( view.viewFrustum.calculateCubeKeyholeIntersection(cube) != ViewFrustum::OUTSIDE) {
                            ViewFrustum::intersection lastIntersection = lastView.viewFrustum.calculateCubeKeyholeIntersection(cube);
                            if (!(lastIntersection == ViewFrustum::INSIDE && nextElement->getLastChanged() < lastView.startTime)) {
                                next.element = nextElement;
                                // NOTE: for differential case next.intersection is against the lastView
                                // because this helps the "external scan" optimize its culling
                                next.intersection = lastIntersection;
                                return;
                            } else {
                                // check for LOD truncation in the last traversal because
                                // we may need to traverse this element after all if the lastView skipped it for LOD
                                int32_t lastLevel = element->getLevel() + lastView.lodLevelOffset;
                                visibleLimit = boundaryDistanceForRenderLevel(lastLevel, lastView.rootSizeScale);
                                distance2 = glm::distance2(lastView.viewFrustum.getPosition(), element->getAACube().calcCenter());
                                if (distance2 >= visibleLimit * visibleLimit) {
                                    next.element = nextElement;
                                    next.intersection = lastIntersection;
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}

DiffTraversal::Type DiffTraversal::prepareNewTraversal(
        const ViewFrustum& viewFrustum, EntityTreeElementPointer root, int32_t lodLevelOffset) {
    assert(root);
    // there are three types of traversal:
    //
    //   (1) First = fresh view --> find all elements in view
    //   (2) Repeat = view hasn't changed --> find elements changed since last complete traversal
    //   (3) Differential = view has changed --> find elements changed or in new view but not old
    //
    // for each traversal type we assign the appropriate _getNextVisibleElementCallback
    //
    //   _getNextVisibleElementCallback = identifies elements that need to be traversed,
    //        updates VisibleElement ref argument with pointer-to-element and view-intersection
    //        (INSIDE, INTERSECT, or OUTSIDE)
    //
    // external code should update the _scanElementCallback after calling prepareNewTraversal
    //

    Type type;
    if (_completedView.startTime == 0) {
        type = Type::First;
        _currentView.viewFrustum = viewFrustum;
        _currentView.lodLevelOffset = root->getLevel() + lodLevelOffset - 1; // -1 because true root has level=1
        _currentView.rootSizeScale = root->getScale() * MAX_VISIBILITY_DISTANCE_FOR_UNIT_ELEMENT;
        _getNextVisibleElementCallback = [&](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementFirstTime(next, _currentView);
        };
    } else if (_completedView.viewFrustum.isVerySimilar(viewFrustum) && lodLevelOffset == _completedView.lodLevelOffset) {
        type = Type::Repeat;
        _getNextVisibleElementCallback = [&](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementRepeat(next, _completedView, _completedView.startTime);
        };
    } else {
        type = Type::Differential;
        _currentView.viewFrustum = viewFrustum;
        _currentView.lodLevelOffset = root->getLevel() + lodLevelOffset - 1; // -1 because true root has level=1
        _currentView.rootSizeScale = root->getScale() * MAX_VISIBILITY_DISTANCE_FOR_UNIT_ELEMENT;
        _getNextVisibleElementCallback = [&](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementDifferential(next, _currentView, _completedView);
        };
    }

    _path.clear();
    _path.push_back(DiffTraversal::Waypoint(root));
    // set root fork's index such that root element returned at getNextElement()
    _path.back().initRootNextIndex();

    _currentView.startTime = usecTimestampNow();

    return type;
}

void DiffTraversal::getNextVisibleElement(DiffTraversal::VisibleElement& next) {
    if (_path.empty()) {
        next.element.reset();
        next.intersection = ViewFrustum::OUTSIDE;
        return;
    }
    _getNextVisibleElementCallback(next);
    if (next.element) {
        int8_t nextIndex = _path.back().getNextIndex();
        if (nextIndex > 0) {
            _path.push_back(DiffTraversal::Waypoint(next.element));
        }
    } else {
        // we're done at this level
        while (!next.element) {
            // pop one level
            _path.pop_back();
            if (_path.empty()) {
                // we've traversed the entire tree
                _completedView = _currentView;
                return;
            }
            // keep looking for next
            _getNextVisibleElementCallback(next);
            if (next.element) {
                // we've descended one level so add it to the path
                _path.push_back(DiffTraversal::Waypoint(next.element));
            }
        }
    }
}

void DiffTraversal::setScanCallback(std::function<void (DiffTraversal::VisibleElement&)> cb) {
    if (!cb) {
        _scanElementCallback = [](DiffTraversal::VisibleElement& a){};
    } else {
        _scanElementCallback = cb;
    }
}

// DEBUG method: delete later
std::ostream& operator<<(std::ostream& s, const DiffTraversal& traversal) {
    for (size_t i = 0; i < traversal._path.size(); ++i) {
        s << (int32_t)(traversal._path[i].getNextIndex());
        if (i < traversal._path.size() - 1) {
            s << "-->";
        }
    }
    return s;
}

void DiffTraversal::traverse(uint64_t timeBudget) {
    uint64_t expiry = usecTimestampNow() + timeBudget;
    DiffTraversal::VisibleElement next;
    getNextVisibleElement(next);
    while (next.element) {
        if (next.element->hasContent()) {
            _scanElementCallback(next);
        }
        if (usecTimestampNow() > expiry) {
            break;
        }
        getNextVisibleElement(next);
    }
}
