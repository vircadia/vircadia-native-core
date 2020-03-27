# JavaScript Documentation Generation

## Prerequisites

* Install node.js.
* Install jsdoc via npm.  `npm install jsdoc -g`

If you would like the extra functionality for gravPrep:
* Run `npm install`

To generate HTML documentation for the Vircadia JavaScript API:

* `cd tools/jsdoc`
* `jsdoc root.js -r api-mainpage.md -c config.json`

The **out** folder should contain index.html.

If you get a "JavaScript heap out of memory" error when running the `jsdoc` command you need to increase the amount of memory 
available to it. For example, to increase the memory available to 2GB on Windows:
* `where jsdoc` to find the `jsdoc.cmd` file.
* Edit the `jsdoc.cmd` file to add `--max-old-space-size=2048` after the `node` and/or `node.exe` commands.

Reference: https://medium.com/@vuongtran/how-to-solve-process-out-of-memory-in-node-js-5f0de8f8464c

To generate the grav automation files, run node gravPrep.js after you have made a JSdoc output folder.

This will create files that are needed for hifi-grav and hifi-grav-content repos

The md files for hifi-grav-content are located in out/grav/06.api-reference.

The template twig html files for hifi-grav are located out/grav/templates.

if you would like to copy these to a local version of the docs on your system you can run with the follows args:

* node grav true "path/to/grav/" "path/to/grav/content"