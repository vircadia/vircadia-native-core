After building auto-tester, it needs to be deployed to Amazon SW

* In folder hifi/build/tools/auto-tester
    * Right click on the Release folder
    * Select 7-Zip -> Add to archive
    * Select Option ```Create SFX archive``` to create Release.exe
* Use Cyberduck (or any other AWS S3 client) to copy Release.exe to hifi-content/nissim/autoTester/