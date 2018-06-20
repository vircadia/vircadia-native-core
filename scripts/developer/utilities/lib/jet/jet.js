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
    if (root.isTask()) { 
       depth++;
        for (var i = 0; i <root.getNumSubs(); i++) {
            var sub = root.getSubConfig(i);
            if (functor(sub, depth, i)) {
                task_traverse(sub, functor, depth, 0)
            }
        }
    }    
}
function task_traverseTree(root, functor) {
    if (functor(root, 0, 0)) {
        task_traverse(root, functor, 0)
    }   
}

// Access job properties
// return all the properties of a job
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

// Use this function to create a functor that will fill the specifed array with one entry name per task and job and it s rank
function job_list_functor(jobList, maxDepth) {
    if (maxDepth === undefined) maxDepth = 100
    return function (job, depth, index) {
        jobList.push(job.objectName);
        return depth < maxDepth;
    }
} 

// Use this function to create a functor that will print the content of the Job visited calling the  specified 'printout' function
function job_print_functor(printout, showProps, maxDepth) {
    if (maxDepth === undefined) maxDepth = 100
    return function (job, depth, index) {
        var tab = "    "
        var depthTab = "";
        for (var d = 0; d < depth; d++) { depthTab += tab }
        printout(depthTab + index + " " + job.objectName + " " + (job.enabled ? "on " : "off ") + job.cpuRunTime + "ms") 
        if (showProps) {
            var keys = job_propKeys(job);
            for (var p=0; p < keys.length;p++) {
                var prop = job[keys[p]]
                printout(depthTab + tab + tab + typeof prop + " " + keys[p] + " " + prop);
            }
        }
        return depth < maxDepth;
    }
} 

// Expose functions for regular js including this files through the 'Jet' object
/*Jet = {}
Jet.task_traverse = task_traverse
Jet.task_traverseTree = task_traverseTree
Jet.job_propKeys = job_propKeys
Jet.job_print_functor = job_print_functor
*/