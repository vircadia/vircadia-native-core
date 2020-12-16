
The project embraces distributed development and if you'd like to help, it would be greatly appreciated. Just open a pull request with the revisions.

Contributing
===
1. Fork the repository to your own GitHub account.
2. Clone your fork of the repository locally

  ```
  git clone git://github.com/USERNAME/vircadia.git
  ```
3. Create a new branch
  
  ```
  git checkout -b new_branch_name 
  ```
4. Code
  * Follow the [coding standard](CODING_STANDARD.md)
5. Commit
  * Use [well formed commit messages](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html)
6. Update your branch
  
  ```
  git remote add upstream https://github.com/vircadia/vircadia
  git pull upstream master
  ```
  
  Resolve any conflicts that arise with this step.
  
7. Push to your fork
  
  ```
  git push origin new_branch_name
  ```
8. Submit a pull request

  *You can follow [GitHub's guide](https://help.github.com/articles/creating-a-pull-request) to find out how to create a pull request.*
  
Tips for Pull Requests 
===
To make the QA process go as smoothly as possible.

1. Have a basic description in your pull request. 
2. Write a basic test plan if you are altering or adding features.
3. If a new API is added, try to make sure that some level of basic documentation on how you can utilize it is included.
4. If an added API or feature requires an external service, try to document or link to instructions on how to create a basic working setup.

Reporting Bugs
===
1. Always update to the latest code on master, we make many merges every day and it is possible the bug has already been fixed!
2. Search [issues](https://github.com/vircadia/vircadia/issues) to make sure that somebody has not already reported the same bug. 
3. [Add](https://github.com/vircadia/vircadia/issues/new) your report to the issues list!

Requesting a Feature
===
1. Search [issues](https://github.com/vircadia/vircadia/issues) to make sure that somebody has not already requested the same feature. 
2. [Add](https://github.com/vircadia/vircadia/issues/new) your request to the issues list!
