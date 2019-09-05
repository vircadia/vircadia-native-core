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

 // traverse task tree recursively
 //
 // @param root: the root job config from where to traverse
 // @param functor: the functor function() which is applied on every subjobs of root traversed
 //                 if return true, then 'task_tree' is called recursively on that subjob
 // @param depth: the depth of the recurse loop since the initial call.      
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

// same function as 'task_traverse' with the depth being 0
// and visisting the root job first.
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
    if (job.isSwitch()) {
        propKeys.push("branch")
    }
    for (var k=0; k < keys.length;k++) {
        // Filter for relevant property
        var key = keys[k]
        if ((typeof job[key]) !== "function") {
            if ((key !== "objectName") && (key !== "cpuRunTime") && (key !== "enabled")  && (key !== "branch")) {
                propKeys.push(keys[k]);
            }
        }
    }   

    return propKeys; 
}

// Access job inputs
// return all the inputs of a job
function job_inoutKeys(job) {
    var keys = Object.keys(job)
    var inoutKeys = [];
    for (var k=0; k < keys.length;k++) {
        // Filter for relevant property
        var key = keys[k]
        if ((typeof job[key]) !== "function") {
            if ((key == "input") || (key == "output")) {
                inoutKeys.push(keys[k]);
            }
        }
    }

    return inoutKeys; 
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
function job_print_functor(printout, showProps, showInOuts, maxDepth) {
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
        if (showInOuts) {
            printout("jsdkfkjdskflj")
            var inouts = job_inoutKeys(job);
            for (var p=0; p < inouts.length;p++) {
                var prop = job[inouts[p]]
                printout(depthTab + tab + tab + typeof prop + " " + inouts[p] + " " + prop);
            }
        }
        return depth < maxDepth;
    }
} 

// Use this function to create a functor that will build a tree datastructure of the Job visited

function job_tree_model_array_functor(jobTreeArray, newNodeFunctor) {
    var jobsRoot;
    var numJobs = 0;
    var jobTreePath = []
    if (newNodeFunctor === undefined) newNodeFunctor = function (node) {}

    return function (job, depth, index) {
        var id = numJobs
        var newItem = {"name": job.objectName, "level": depth, "index": index, "id": id, "subNode": [], "path": ""}
        if (depth == 0) {
            newNodeFunctor(newItem)
            jobTreeArray.push(newItem)
            numJobs++
            jobsRoot = jobTreeArray[0].subNode;
        } else {
            if (jobTreePath.length < depth) {
                var node = jobsRoot;
                var path;
                for (var n = 0; n < jobTreePath.length; n++) {
                    newItem.path += (n > 0 ? "." : "") + node[jobTreePath[n]].name
                    node = node[jobTreePath[n]].subNode
                }
                
                newNodeFunctor(newItem)
                node.push((newItem))
                numJobs++
                jobTreePath.push(0);
            } else if (jobTreePath.length >= depth) {
                var node = jobsRoot;
                for (var n = 0; n < (depth - 1); n++) {
                    newItem.path += (n > 0 ? "." : "") + node[jobTreePath[n]].name
                    node = node[jobTreePath[n]].subNode
                }

                newNodeFunctor(newItem)
                node.push((newItem))
                numJobs++
                jobTreePath[depth-1] = index;
                while (jobTreePath.length > depth) {
                    jobTreePath.pop();
                }                       
            }
        }
        return true;
    }
}

function job_tree_model_functor(jobTreeModel, maxLevel, newNodeFunctor) {
    var jobsRoot;
    var numJobs = 0;
    var jobTreePath = []
    if (newNodeFunctor === undefined) newNodeFunctor = function (node) {}

    return function (job, depth, index) {
        var id = numJobs
        var newItem = {"name": job.objectName, "level": depth, "index": index, "id": id, "subNode": [], "path": "", "init": (depth < maxLevel), "ud": {}}
        if (depth == 0) {
            newNodeFunctor(newItem)
            jobTreeModel.append(newItem)
            numJobs++
            jobsRoot = jobTreeModel.get(0).subNode;
        } else {
            if (jobTreePath.length < depth) {
                var node = jobsRoot;
                var path;
                for (var n = 0; n < jobTreePath.length; n++) {
                    newItem.path += (n > 0 ? "." : "") + node.get(jobTreePath[n]).name
                    node = node.get(jobTreePath[n]).subNode
                }
                
                newNodeFunctor(newItem)
                node.append((newItem))
                numJobs++
                jobTreePath.push(0);
            } else if (jobTreePath.length >= depth) {
                var node = jobsRoot;
                for (var n = 0; n < (depth - 1); n++) {
                    newItem.path += (n > 0 ? "." : "") + node.get(jobTreePath[n]).name
                    node = node.get(jobTreePath[n]).subNode
                }

                newNodeFunctor(newItem)
                node.append((newItem))
                numJobs++
                jobTreePath[depth-1] = index;
                while (jobTreePath.length > depth) {
                    jobTreePath.pop();
                }                       
            }
        }
        return true;
    }
}


// Traverse the jobTreenode data structure created above
function job_traverseTreeNode(root, functor, depth) {
    if (root.subNode.length) { 
        depth++;
        for (var i = 0; i <root.subNode.length; i++) {
            var sub = root.subNode[i];
            if (functor(sub, depth, i)) {
                job_traverseTreeNode(sub, functor, depth, 0)
            }
        }
    }
}

function job_traverseTreeNodeRoot(root, functor) {
    if (functor(root, 0, 0)) {
        job_traverseTreeNode(root, functor, 0)
    }   
}
// Expose functions for regular js including this files through the 'Jet' object
/*Jet = {}
Jet.task_traverse = task_traverse
Jet.task_traverseTree = task_traverseTree
Jet.job_propKeys = job_propKeys
Jet.job_print_functor = job_print_functor
*/