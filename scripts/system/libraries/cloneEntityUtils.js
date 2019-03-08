"use strict";

//  cloneEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global entityIsCloneable:true, getGrabbableData:true, cloneEntity:true, propsAreCloneDynamic:true, Script,
   propsAreCloneDynamic:true, Entities*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");

// Object assign  polyfill
if (typeof Object.assign !== 'function') {
    Object.assign = function(target, varArgs) {
        if (target === null) {
            throw new TypeError('Cannot convert undefined or null to object');
        }
        var to = Object(target);
        for (var index = 1; index < arguments.length; index++) {
            var nextSource = arguments[index];
            if (nextSource !== null) {
                for (var nextKey in nextSource) {
                    if (Object.prototype.hasOwnProperty.call(nextSource, nextKey)) {
                        to[nextKey] = nextSource[nextKey];
                    }
                }
            }
        }
        return to;
    };
}

entityIsCloneable = function(props) {
    if (props) {
        return props.cloneable;
    }
    return false;
};

propsAreCloneDynamic = function(props) {
    var cloneable = entityIsCloneable(props);
    if (cloneable) {
        return props.cloneDynamic;
    }
    return false;
};

cloneEntity = function(props) {
    var entityToClone = props.id;
    var props = Entities.getEntityProperties(entityToClone, ['certificateID', 'certificateType'])
    var certificateID = props.certificateID;
    // ensure entity is cloneable and does not have a certificate ID, whereas cloneable limits
    // will now be handled by the server where the entity add will fail if limit reached
    if (entityIsCloneable(props) && (!!certificateID || props.certificateType.indexOf('domainUnlimited') >= 0)) {
        var cloneID = Entities.cloneEntity(entityToClone);
        return cloneID;
    }
    return null;
};
