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

 // traverse task tree
function task_traverse(root, functor, depth) {
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
function task_traverseTree(root, functor) {
    if (functor(root, 0, 0)) {
        task_traverse(root, functor, 0)
    }   
}

// Access job properties
function job_propKeys(job) {
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

function job_print_functor(printout, maxDepth) {
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

function getRender() {
    return Render
}


function getWorkload() {
    return Workload
}