#!/bin/bash
hifi
git pull upstream master
jsd
rm -rf out
jsdoc root.js -c config.json
node gravPrep true "D:\hifi-docs-grav\user\themes\learn2" "D:\hifi-docs-grav-content"
pages
git add .
git commit -m "adding new pages"
git push origin new-pages
learn2
git add .
git commit -m "adding new content"
git push origin update