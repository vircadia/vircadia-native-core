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
    AssignmentClientChildData(QString childType);

    QString getChildType() { return _childType; }
    void setChildType(QString childType) { _childType = childType; }

    // implement parseData to return 0 so we can be a subclass of NodeData
    int parseData(const QByteArray& packet) { return 0; }

 private:
    QString _childType;
};

#endif // hifi_AssignmentClientChildData_h
