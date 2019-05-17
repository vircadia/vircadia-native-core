//
//  Simplified Nametag
//  entityMaker.js
//  Created by Milad Nazeri on 2019-02-19
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  A helper library to make entities
//


Script.require('./objectAssign.js');

function EntityMaker(type) {
    this.properties = {};
    this.cache = {};
    this.id = null;
    this.created = null;
    this.type = type;
}


// *************************************
// START API
// *************************************
// #region API


// Add properties to the cache / temporary storage
function add(props){
    // You can either add an object of props or 2 arguments as key and value
    if (arguments.length === 2) {
        var property = arguments[0];
        var value = arguments[1];
        props = {};
        props[property] = value;
    }

    this.properties = Object.assign({}, this.properties, props);
    this.cache = Object.assign({}, this.cache, this.properties);

    return this;
}


// Sends the current temporary stroage to edit the entity
function sync(){
    Entities.editEntity(this.id, this.properties);
    this.properties = {};

    return this;
}


// Immediately edit the entity with the properties given
function edit(props){
    if (arguments.length === 2) {
        var property = arguments[0];
        var value = arguments[1];
        props = {};
        props[property] = value;
    }
    this.properties = Object.assign({}, this.properties, props);
    this.cache = Object.assign({}, this.cache, this.properties);
    this.sync();

    return this;
}


// Get a property either from the cache or by querying the entity directly
function get(propertyKeys, queryEntity){
    if (queryEntity && typeof propertyKeys === 'string') {
        var propertyValue = Entities.getEntityProperties(this.id, propertyKeys)[propertyKeys];
        this.properties[propertyKeys] = propertyValue;
        this.cache = Object.assign({}, this.cache, this.properties);
        return propertyValue;
    }

    if (queryEntity && Array.isArray(propertyKeys)) {
        var entityProps = Entities.getEntityProperties(this.id, propertyKeys);
        for (var prop in entityProps) {
            if (propertyKeys.indexOf(prop) === -1) {
                delete entityProps[prop];
            } else {
                this.properties[prop] = entityProps[prop];
            }
        }
        return entityProps;
    }

    if (Array.isArray(propertyKeys)) {
        var recombinedProps = {};
        propertyKeys.forEach(function (prop) {
            recombinedProps[prop] = this.cache[prop];
        }, this);
        return recombinedProps;
    }

    return this.cache[propertyKeys];
}


// Show the entity
function show(){
    this.edit({ visible: true });

    return this;
}


// Hide the enity
function hide(){
    this.edit({ visible: false });
}


// Add an entity if it isn't created
function create(clearPropertiesAfter){
    this.id = Entities.addEntity(this.properties, this.type);
    if (clearPropertiesAfter) {
        this.properties = {};
    }
    return this;
}


// Delete the entity
function destroy(){
    Entities.deleteEntity(this.id);

    return this;
}


// #endregion
// *************************************
// END API
// *************************************

EntityMaker.prototype = {
    add: add, 
    sync: sync, 
    edit: edit,
    get: get, 
    show: show,
    hide: hide,
    create: create,
    destroy: destroy
};

module.exports = EntityMaker;