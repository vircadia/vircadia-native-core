//
//  Job Engine & Task...
//  jet.js
//
//  Created by Sam Gateau, 2018/03/28
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
"use strict";
/*function Jet() {
};
Jet.prototype = {
    // traverse task tree
    task_traverse: function(root, functor, depth) {
        var subs = root.findChildren(/.*//*)
        depth++;
        for (var i = 0; i <subs.length; i++) {
            if (functor(subs[i], depth, i)) {
                this.task_traverse(subs[i], functor, depth)
            }
        }    
    },   
    task_traverseTree: function(root, functor) {
        if (functor(root, 0, 0)) {
            this.task_traverse(root, functor, 0)
        } 
    },

    // Access job properties
    job_propKeys: function(job) {
        var keys = Object.keys(job)
        var propKeys = [];
        for (var k=0; k < keys.length;k++) {
            // Filter for relevant property
            var key = keys[k]
            if ((typeof job[key]) !== "function") {
                if ((key !== "objectName") && (key !== "cpuRunTime") && (key !== "enabled")) {
                    propKeys.push(keys[k]);
                }
            }
        }   
        return propKeys; 
    }
};

jet = Jet();

// Functors
jet.job_print_functor = function (printout, maxDepth) {
    if (maxDepth === undefined) maxDepth = 100
    return function (job, depth, index) {
        var tab = "    "
        var depthTab = "";
        for (var d = 0; d < depth; d++) { depthTab += tab }
        printout(depthTab + index + " " + job.objectName + " " + (job.enabled ? "on" : "off")) 
        var keys = jet.job_propKeys(job);
        for (var p=0; p < keys.length;p++) {
            var prop = job[keys[p]]
            printout(depthTab + tab + tab + typeof prop + " " + keys[p] + " " + prop);
        }

        return depth < maxDepth;
    }
} 
*/
 // traverse task tree
task_traverse = function (root, functor, depth) {
    //if (root.isTask()) { 
        var subs = root.getSubConfigs()
        depth++;
        for (var i = 0; i <subs.length; i++) {
            if (functor(subs[i], depth, i)) {
                task_traverse(subs[i], functor, depth)
            }
        }
   // }    
}
task_traverseTree = function (root, functor) {
    if (functor(root, 0, 0)) {
        task_traverse(root, functor, 0)
    }   
}

// Access job properties
job_propKeys = function (job) {
var keys = Object.keys(job)
var propKeys = [];
for (var k=0; k < keys.length;k++) {
    // Filter for relevant property
    var key = keys[k]
    if ((typeof job[key]) !== "function") {
        if ((key !== "objectName") && (key !== "cpuRunTime") && (key !== "enabled")) {
            propKeys.push(keys[k]);
        }
    }
}   
return propKeys; 
}
job_print_functor = function (printout, maxDepth) {
    if (maxDepth === undefined) maxDepth = 100
    return function (job, depth, index) {
        var tab = "    "
        var depthTab = "";
        for (var d = 0; d < depth; d++) { depthTab += tab }
        printout(depthTab + index + " " + job.objectName + " " + (job.enabled ? "on" : "off")) 
        var keys = job_propKeys(job);
        for (var p=0; p < keys.length;p++) {
            var prop = job[keys[p]]
            printout(depthTab + tab + tab + typeof prop + " " + keys[p] + " " + prop);
        }

        return true
     //   return depth < maxDepth;
    }
} 

//jet.task_traverseTree(Render, jet.job_print_functor);
