//
//  AssignmentClientChildData.h
//  assignment-client/src
//
//  Created by Seth Alves on 2/23/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssignmentClientChildData_h
#define hifi_AssignmentClientChildData_h

#include <Assignment.h>


class AssignmentClientChildData : public NodeData {
public:
    AssignmentClientChildData(Assignment::Type childType);

    Assignment::Type getChildType() { return _childType; }
    void setChildType(Assignment::Type childType) { _childType = childType; }

private:
    Assignment::Type _childType;
};

#endif // hifi_AssignmentClientChildData_h
