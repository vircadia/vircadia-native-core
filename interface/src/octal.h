//
//  octal.h
//  interface
//
//  Created by Philip on 2/4/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __interface__octal__
#define __interface__octal__

#include <iostream>

struct domainNode {
    domainNode * child[8];
    char * hostname;
    char * nickname;
    int domain_id;
};

domainNode* createNode(int lengthInBits, char * octalData,
                       char * hostname, char * nickname, int domain_id);



#endif /* defined(__interface__octal__) */
