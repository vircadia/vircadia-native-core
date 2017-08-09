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

DiffTraversal::Waypoint::Waypoint(EntityTreeElementPointer& element) : _nextIndex(0) {
    assert(element);
    _weakElement = element;
}

void DiffTraversal::Waypoint::getNextVisibleElementFirstTime(DiffTraversal::VisibleElement& next, const ViewFrustum& view) {
    // TODO: add LOD culling of elements
    // NOTE: no need to set next.intersection in the "FirstTime" context
    if (_nextIndex == -1) {
        // only get here for the root Waypoint at the very beginning of traversal
        // safe to assume this element intersects view
        ++_nextIndex;
        next.element = _weakElement.lock();
        return;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && view.cubeIntersectsKeyhole(nextElement->getAACube())) {
                    next.element = nextElement;
                    return;
                }
            }
        }
    }
    next.element.reset();
}

void DiffTraversal::Waypoint::getNextVisibleElementRepeat(DiffTraversal::VisibleElement& next, const ViewFrustum& view, uint64_t lastTime) {
    // TODO: add LOD culling of elements
    if (_nextIndex == -1) {
        // only get here for the root Waypoint at the very beginning of traversal
        // safe to assume this element intersects view
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        // root case is special: its intersection is always INTERSECT
        // and we can skip it if the content hasn't changed
        if (element->getLastChangedContent() > lastTime) {
            next.element = element;
            next.intersection = ViewFrustum::INTERSECT;
            return;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && nextElement->getLastChanged() > lastTime) {
                    ViewFrustum::intersection intersection = view.calculateCubeKeyholeIntersection(nextElement->getAACube());
                    if (intersection != ViewFrustum::OUTSIDE) {
                        next.element = nextElement;
                        next.intersection = intersection;
                        return;
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
        const ViewFrustum& view, const ViewFrustum& lastView, uint64_t lastTime) {
    // TODO: add LOD culling of elements
    if (_nextIndex == -1) {
        // only get here for the root Waypoint at the very beginning of traversal
        // safe to assume this element intersects view
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        // root case is special: its intersection is always INTERSECT
        // and we can skip it if the content hasn't changed
        if (element->getLastChangedContent() > lastTime) {
            next.element = element;
            next.intersection = ViewFrustum::INTERSECT;
            return;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement) {
                    AACube cube = nextElement->getAACube();
                    // NOTE: for differential case next.intersection is against the _completedView
                    ViewFrustum::intersection intersection = lastView.calculateCubeKeyholeIntersection(cube);
                    if ( lastView.calculateCubeKeyholeIntersection(cube) != ViewFrustum::OUTSIDE &&
                            !(intersection == ViewFrustum::INSIDE && nextElement->getLastChanged() < lastTime)) {
                        next.element = nextElement;
                        next.intersection = intersection;
                        return;
                    }
                }
            }
        }
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}

DiffTraversal::Type DiffTraversal::prepareNewTraversal(const ViewFrustum& viewFrustum, EntityTreeElementPointer root) {
    // there are three types of traversal:
    //
    //      (1) First = fresh view --> find all elements in view
    //      (2) Repeat = view hasn't changed --> find elements changed since last complete traversal
    //      (3) Differential = view has changed --> find elements changed or in new view but not old
    //
    // For each traversal type we define assign the appropriate _getNextVisibleElementCallback
    //
    // _getNextVisibleElementCallback = identifies elements that need to be traversed,
    //      updates VisibleElement ref argument with pointer-to-element and view-intersection
    //      (INSIDE, INTERSECT, or OUTSIDE)
    //
    // External code should update the _scanElementCallback after calling prepareNewTraversal
    //

    Type type = Type::First;
    if (_startOfCompletedTraversal == 0) {
        // first time
        _currentView = viewFrustum;
        _getNextVisibleElementCallback = [&](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementFirstTime(next, _currentView);
        };
    } else if (_currentView.isVerySimilar(viewFrustum)) {
        // again
        _getNextVisibleElementCallback = [&](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementRepeat(next, _currentView, _startOfCompletedTraversal);
        };
        type = Type::Repeat;
    } else {
        // differential
        _currentView = viewFrustum;
        _getNextVisibleElementCallback = [&](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementDifferential(next, _currentView, _completedView, _startOfCompletedTraversal);
        };
        type = Type::Differential;
    }

    assert(root);
    _path.clear();
    _path.push_back(DiffTraversal::Waypoint(root));
    // set root fork's index such that root element returned at getNextElement()
    _path.back().initRootNextIndex();
    _startOfCurrentTraversal = usecTimestampNow();

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
            // next.element needs to be added to the path
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
                _startOfCompletedTraversal = _startOfCurrentTraversal;
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
