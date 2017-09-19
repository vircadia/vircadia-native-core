"use strict";

//  cloneEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global entityIsCloneable:true, getGrabbableData:true, cloneEntity:true, propsAreCloneDynamic:true, Script,
   propsAreCloneDynamic:true, Entities*/

Script.include("/~/system/controllers/controllerDispatcherUtils.js");


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
        var grabbableData = getGrabbableData(props);
        return grabbableData.cloneable;
    }
    return false;
};

propsAreCloneDynamic = function(props) {
    var cloneable = entityIsCloneable(props);
    if (cloneable) {
        var grabInfo = getGrabbableData(props);
        if (grabInfo.cloneDynamic) {
            return true;
        }
    }
    return false;
};


cloneEntity = function(props, worldEntityProps) {
    // we need all the properties, for this
    var cloneableProps = Entities.getEntityProperties(props.id);

    var count = 0;
    worldEntityProps.forEach(function(itemWE) {
        if (itemWE.name.indexOf('-clone-' + cloneableProps.id) !== -1) {
            count++;
        }
    });

    var grabInfo = getGrabbableData(cloneableProps);
    var limit = grabInfo.cloneLimit ? grabInfo.cloneLimit : 0;
    if (count >= limit && limit !== 0) {
        return null;
    }

    cloneableProps.name = cloneableProps.name + '-clone-' + cloneableProps.id;
    var lifetime = grabInfo.cloneLifetime ? grabInfo.cloneLifetime : 300;
    var dynamic = grabInfo.cloneDynamic ? grabInfo.cloneDynamic : false;
    var triggerable = grabInfo.triggerable ? grabInfo.triggerable : false;
    var avatarEntity = grabInfo.cloneAvatarEntity ? grabInfo.cloneAvatarEntity : false;
    var cUserData = Object.assign({}, JSON.parse(cloneableProps.userData));
    var cProperties = Object.assign({}, cloneableProps);


    delete cUserData.grabbableKey.cloneLifetime;
    delete cUserData.grabbableKey.cloneable;
    delete cUserData.grabbableKey.cloneDynamic;
    delete cUserData.grabbableKey.cloneLimit;
    delete cUserData.grabbableKey.cloneAvatarEntity;
    delete cProperties.id;


    cProperties.dynamic = dynamic;
    cProperties.locked = false;
    cUserData.grabbableKey.triggerable = triggerable;
    cUserData.grabbableKey.grabbable = true;
    cProperties.lifetime = lifetime;
    cProperties.userData = JSON.stringify(cUserData);

    var cloneID = Entities.addEntity(cProperties, avatarEntity);
    return cloneID;
};
