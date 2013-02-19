//
//  TreeNode.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/13/13.
//
//

#include "TreeNode.h"

std::string EMPTY_STRING = "";

TreeNode::TreeNode() {
    for (int i = 0; i < CHILDREN_PER_NODE; ++i) {
        child[i] = NULL;
    }
    
    hostname = NULL;
    nickname = NULL;
}