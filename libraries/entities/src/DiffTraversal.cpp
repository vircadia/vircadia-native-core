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
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement) {
                    const auto& cube = nextElement->getAACube();
                    if (!view.usesViewFrustums()) {
                        // No LOD truncation if we aren't using the view frustum
                        next.element = nextElement;
                        return;
                    } else if (view.intersects(cube)) {
                        // check for LOD truncation
                        if (view.isBigEnough(cube, MIN_ELEMENT_ANGULAR_DIAMETER)) {
                            next.element = nextElement;
                            return;
                        }
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
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && nextElement->getLastChanged() > lastTime) {
                    if (!view.usesViewFrustums()) {
                        // No LOD truncation if we aren't using the view frustum
                        next.element = nextElement;
                        next.intersection = ViewFrustum::INSIDE;
                        return;
                    } else {
                        // check for LOD truncation
                        const auto& cube = nextElement->getAACube();
                        if (view.isBigEnough(cube, MIN_ELEMENT_ANGULAR_DIAMETER)) {
                            ViewFrustum::intersection intersection = view.calculateIntersection(cube);
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
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}

void DiffTraversal::Waypoint::getNextVisibleElementDifferential(DiffTraversal::VisibleElement& next,
        const DiffTraversal::View& view, const DiffTraversal::View& lastView) {
    if (_nextIndex == -1) {
        // root case is special
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        next.element = element;
        next.intersection = ViewFrustum::INTERSECT;
        return;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement) {
                    // check for LOD truncation
                    const auto& cube = nextElement->getAACube();
                    if (view.isBigEnough(cube, MIN_ELEMENT_ANGULAR_DIAMETER)) {
                        ViewFrustum::intersection intersection = view.calculateIntersection(cube);
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

bool DiffTraversal::View::isBigEnough(const AACube& cube, float minDiameter) const {
    if (viewFrustums.empty()) {
        // Everything is big enough when not using view frustums
        return true;
    }

    bool isBigEnough = std::any_of(std::begin(viewFrustums), std::end(viewFrustums),
                                   [&](const ViewFrustum& viewFrustum) {
        return isAngularSizeBigEnough(viewFrustum.getPosition(), cube, lodScaleFactor, minDiameter);
    });

    return isBigEnough;
}

bool DiffTraversal::View::intersects(const AACube& cube) const {
    if (viewFrustums.empty()) {
        // Everything intersects when not using view frustums
        return true;
    }

    bool intersects = std::any_of(std::begin(viewFrustums), std::end(viewFrustums),
                                  [&](const ViewFrustum& viewFrustum) {
        return viewFrustum.cubeIntersectsKeyhole(cube);
    });

    return intersects;
}

ViewFrustum::intersection DiffTraversal::View::calculateIntersection(const AACube& cube) const {
    if (viewFrustums.empty()) {
        // Everything is inside when not using view frustums
        return ViewFrustum::INSIDE;
    }

    ViewFrustum::intersection intersection = ViewFrustum::OUTSIDE;

    for (const auto& viewFrustum : viewFrustums) {
        switch (viewFrustum.calculateCubeKeyholeIntersection(cube)) {
            case ViewFrustum::INSIDE:
                return ViewFrustum::INSIDE;
            case ViewFrustum::INTERSECT:
                intersection = ViewFrustum::INTERSECT;
                break;
            default:
                // DO NOTHING
                break;
        }
    }
    return intersection;
}

bool DiffTraversal::View::usesViewFrustums() const {
    return !viewFrustums.empty();
}

bool DiffTraversal::View::isVerySimilar(const View& view) const {
    auto size = view.viewFrustums.size();

    if (view.lodScaleFactor != lodScaleFactor ||
        viewFrustums.size() != size) {
        return false;
    }

    for (size_t i = 0; i < size; ++i) {
        if (!viewFrustums[i].isVerySimilar(view.viewFrustums[i])) {
            return false;
        }
    }
    return true;
}

DiffTraversal::DiffTraversal() {
    const int32_t MIN_PATH_DEPTH = 16;
    _path.reserve(MIN_PATH_DEPTH);
}

DiffTraversal::Type DiffTraversal::prepareNewTraversal(const DiffTraversal::View& view, EntityTreeElementPointer root) {
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
    // If usesViewFrustum changes, treat it as a First traversal
    if (_completedView.startTime == 0 || _currentView.usesViewFrustums() != _completedView.usesViewFrustums()) {
        type = Type::First;
        _currentView.viewFrustums = view.viewFrustums;
        _currentView.lodScaleFactor = view.lodScaleFactor;
        _getNextVisibleElementCallback = [this](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementFirstTime(next, _currentView);
        };
    } else if (!_currentView.usesViewFrustums() || _completedView.isVerySimilar(view)) {
        type = Type::Repeat;
        _getNextVisibleElementCallback = [this](DiffTraversal::VisibleElement& next) {
            _path.back().getNextVisibleElementRepeat(next, _completedView, _completedView.startTime);
        };
    } else {
        type = Type::Differential;
        _currentView.viewFrustums = view.viewFrustums;
        _currentView.lodScaleFactor = view.lodScaleFactor;
        _getNextVisibleElementCallback = [this](DiffTraversal::VisibleElement& next) {
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
