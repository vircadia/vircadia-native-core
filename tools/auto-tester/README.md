# Auto Tester

The auto-tester is a stand alone application that provides a mechanism for regression testing.  The general idea is simple:
* Each test folder has a script that produces a set of snapshots.
* The snapshots are compared to a 'canonical' set of images that have been produced beforehand.
* The result, if any test failed, is a zipped folder describing the failure.

Auto-tester has 4 functions, separated into 4 tabs:
1. Creating tests, MD files and recursive scripts
2. Evaluating the results of running tests
3. TestRail interface
4. Windows task bar utility (Windows only)


# Create
![](./Create.png)
The Create tab provides functions to create tests from snapshots, MD files, a test outline and recursive scripts.
## Create Tests
### Usage
This function is used to create/update Expected Images after a successful run of a test, or multiple tests.

The user will be asked for the snapshot folder and then the tests root folder.  All snapshots located in the snapshot folder will be used to create or update the expected images in the relevant tests.
### Details
As an example - if the snapshots folder contains an image named `tests.content.entity.zone.zoneOrientation.00003.png`, then this file will be copied to `tests/contente/enity/zone/zoneOrientation/ExpectedImage0003.png`.
## Create MD file
### Usage
This function creates a file named `test.md` from a `test.js` script.  The user will be asked for the folder containing the test script:
### Details
The process to produce the MD file is a simplistic parse of the test script.
- The string in the `autoTester.perform(...)` function call will be the title of the file

- Instructions to run the script are then provided: 

**Run this script URL: [Manual]()   [Auto]()(from menu/Edit/Open and Run scripts from URL...).**

- The step description is the string in the addStep/addStepStepSnapshot commands

- Image links are provided where applicable to the local Expected Images files
## Create all MD files
### Usage
This function creates all MD files recursively from the user-selected root folder.  This can be any folder in the tests hierarchy (e.g. all engine\material tests).

The file provides a hierarchial list of all the tests 
## Create Tests Outline
### Usage
This function creates an MD file in the (user-selected) tests root folder.  The file provides links to both the tests and the MD files.
## Create Recursive Script
### Usage
After the user selects a folder within the tests hierarchy, a script is created, named `testRecursive.js`.  This script calls all `test.js` scripts in the subfolders.
### Details
The various scripts are called in alphabetical order.

An example of a recursive script is as follows:  
```
// This is an automatically generated file, created by auto-tester on Jul 5 2018, 10:19

PATH_TO_THE_REPO_PATH_UTILS_FILE = "https://raw.githubusercontent.com/highfidelity/hifi_tests/master/tests/utils/branchUtils.js";
Script.include(PATH_TO_THE_REPO_PATH_UTILS_FILE);
var autoTester = createAutoTester(Script.resolvePath("."));

var testsRootPath = autoTester.getTestsRootPath();

if (typeof Test !== 'undefined') {
    Test.wait(10000);
};

autoTester.enableRecursive();
autoTester.enableAuto();

Script.include(testsRootPath + "content/overlay/layer/drawInFront/shape/test.js");
Script.include(testsRootPath + "content/overlay/layer/drawInFront/model/test.js");
Script.include(testsRootPath + "content/overlay/layer/drawHUDLayer/test.js");

autoTester.runRecursive();
```
## Create all Recursive Scripts
### Usage
In this case all recursive scripts, from the selected folder down, are created.

Running this function in the tests root folder will create (or update) all the recursive scripts.
# Evaluate
![](./Evaluate.png)
The Evaluate tab provides a single function - evaluating the results of a test run.

A checkbox (defaulting to checked) runs the evaluation in interactive mode.  In this mode - every failure is shown to the user, who can then decide whether to pass the test, fail it or abort the whole evaluation.

If any tests have failed, then a zipped folder will be created in the snapshots folder, with a description of each failed step in each test.
### Usage
Before starting the evaluation, make sure the GitHub user and branch are set correctly.  The user should not normally be changed, but the branch may need to be set to the appropriate RC.

After setting the checkbox as required and pressing Evaluate - the user will be asked for the snapshots folder.
### Details
Evaluation proceeds in a number of steps:

1. A folder is created to store any failures

1. The expecetd images are download from GitHub.  They are named slightly differently from the snapshots (e.g. `tests.engine.render.effect.highlight.coverage.00000.png` and `tests.engine.render.effect.highlight.coverage.00000_EI.png`).

1. The images are then pair-wise compared, using the SSIM algorithm.  A fixed threshold is used to define a mismatch.

1.  In interactive mode - a window is opened showing the expected image, actual image, difference image and error.

1.  If not in interactive mode, or the user has defined the results as an error, an error is written into the error folder.  The error itself is a folder with the 3 images and a small text file containing details.

1.  At the end of the test, the folder is zipped and the original folder is deleted.  If there are no errors, then there will be no zipped folder.
# TestRail
![](./TestRail.png)
# Windows
![](./Windows.png)