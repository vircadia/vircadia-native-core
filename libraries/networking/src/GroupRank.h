//
//  GroupRank.h
//  libraries/networking/src/
//
//  Created by Seth Alves on 2016-7-21.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GroupRank_h
#define hifi_GroupRank_h

class GroupRank {
public:
    GroupRank() : id(QUuid()), order(-1), name(""), membersCount(-1) {}
    GroupRank(QUuid id, unsigned int order, QString name, unsigned int membersCount) :
        id(id), order(order), name(name), membersCount(membersCount) {}

    QUuid id;
    int order;
    QString name;
    int membersCount;
};

inline bool operator==(const GroupRank& lhs, const GroupRank& rhs) {
    return
        lhs.id == rhs.id &&
        lhs.order == rhs.order &&
        lhs.name == rhs.name &&
        lhs.membersCount == rhs.membersCount;
}
inline bool operator!=(const GroupRank& lhs, const GroupRank& rhs) { return !(lhs == rhs); }

#endif // hifi_GroupRank_h
