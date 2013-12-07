//
//  AttributeRegistry.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AttributeRegistry__
#define __interface__AttributeRegistry__

#include <QHash>
#include <QString>

typedef int AttributeID;

class Attribute;

/// Maintains information about metavoxel attribute types.
class AttributeRegistry {
public:
    
    /// Returns a pointer to the singleton registry instance.
    static AttributeRegistry* getInstance() { return &_instance; }
    
    AttributeRegistry();
    
    /// Returns a unique id for the described attribute, creating one if necessary.
    AttributeID getAttributeID(const QString& name);
    
    /// Returns the identified attribute, or an invalid attribute if not found.
    Attribute getAttribute(AttributeID id) const;
    
private:

    static AttributeRegistry _instance;

    AttributeID _lastAttributeID;
    QHash<QString, AttributeID> _attributeIDs;
    QHash<AttributeID, Attribute> _attributes;    
};

/// Represents a registered attribute.
class Attribute {
public:

    Attribute(AttributeID id = 0, const QString& name = QString());

    bool isValid() const { return !_name.isNull(); }

    AttributeID getID() const { return _id; }
    const QString& getName() const { return _name; }

private:

    AttributeID _id;
    QString _name;
};

#endif /* defined(__interface__AttributeRegistry__) */
