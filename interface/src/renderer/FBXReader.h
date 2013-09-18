//
//  FBXReader.h
//  interface
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__FBXReader__
#define __interface__FBXReader__

#include <QVariant>
#include <QVector>

class QIODevice;

class FBXNode;

typedef QList<FBXNode> FBXNodeList;

/// A node within an FBX document.
class FBXNode {
public:
    
    QByteArray name;
    QVariantList properties;
    FBXNodeList children;
};

/// Parses the input from the supplied data as an FBX file.
/// \exception QString if an error occurs in parsing
FBXNode parseFBX(const QByteArray& data);

/// Parses the input from the supplied device as an FBX file.
/// \exception QString if an error occurs in parsing
FBXNode parseFBX(QIODevice* device);

void printNode(const FBXNode& node, int indent = 0);

#endif /* defined(__interface__FBXReader__) */
