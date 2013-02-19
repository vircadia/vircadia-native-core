//
//  TreeNode.h
//  hifi
//
//  Created by Stephen Birarda on 2/13/13.
//
//

#ifndef __hifi__TreeNode__
#define __hifi__TreeNode__

#include <iostream>

const int CHILDREN_PER_NODE = 8;

class TreeNode {
public:
    TreeNode();
    
    TreeNode *child[CHILDREN_PER_NODE];
    char *hostname;
    char *nickname;
    int domain_id;
};

#endif /* defined(__hifi__TreeNode__) */
