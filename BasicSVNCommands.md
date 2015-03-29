## Basic Usages ##
### 1. Checkout ###
Checkout the repository into a new directory:
```
$ svn checkout https://chrome-screen-capture.googlecode.com/svn/trunk chrome-screen-capture --username your-name
```
### 2. Develop ###
After creating a new file, you can mark it to be added:
```
$ svn add a.png
```
You can also mark an existing file to be deleted:
```
$ svn rm page.js
```
To move or rename a file:
```
$ svn mv a.png images/b.png
```
To revert a modified file, an added file or a deleted file:
```
$ svn revert images/b.png
```
### 3. Update ###
Update to the HEAD revision:
```
$ svn update
```
### 4. Commit ###
Commit code changes to repository:
```
$ svn commit
```
## Branching ##
### 1. Create branch ###
Create a new branch:
```
$ svn cp https://chrome-screen-capture.googlecode.com/svn/trunk https://chrome-screen-capture.googlecode.com/svn/branches/your-new-branch
```
### 2. Switch to branch ###
Switch to the branch you created:
```
$ svn switch https://chrome-screen-capture.googlecode.com/svn/branches/your-new-branch
```
Now you can make code changes and commit to the branch.
### 3. Merge to main branch ###
Print commit logs since branched:
```
$ svn log --stop-on-copy
```
Record the last printed revision, referred as `X` later. Switch back and update to main branch:
```
$ svn switch https://chrome-screen-capture.googlecode.com/svn/trunk
$ svn update
```
Merge:
```
$ svn merge -r X-1:head https://chrome-screen-capture.googlecode.com/svn/branches/your-new-branch
$ svn commit
```
## Tagging ##
Create a new tag:
```
$ svn cp https://chrome-screen-capture.googlecode.com/svn/trunk https://chrome-screen-capture.googlecode.com/svn/tags/release-1.0.0
```