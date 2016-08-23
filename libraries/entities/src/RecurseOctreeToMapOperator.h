//
//  RecurseOctreeToMapOperator.h
//  libraries/entities/src
//
//  Created by Seth Alves on 3/16/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityTree.h"

class RecurseOctreeToMapOperator : public RecurseOctreeOperator {
public:
    RecurseOctreeToMapOperator(QVariantMap& map, OctreeElementPointer top, QScriptEngine* engine, bool skipDefaultValues,
                               bool skipThoseWithBadParents);
    bool preRecursion(OctreeElementPointer element) override;
    bool postRecursion(OctreeElementPointer element) override;
 private:
    QVariantMap& _map;
    OctreeElementPointer _top;
    QScriptEngine* _engine;
    bool _withinTop;
    bool _skipDefaultValues;
    bool _skipThoseWithBadParents;
};
