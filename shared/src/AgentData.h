//
//  AgentData.h
//  hifi
//
//  Created by Stephen Birarda on 2/19/13.
//
//

#ifndef hifi_AgentData_h
#define hifi_AgentData_h

class AgentData {
    public:
        virtual ~AgentData() = 0;
        virtual void parseData(void * data, int size) = 0;
        virtual AgentData* clone() const = 0;
};

#endif
