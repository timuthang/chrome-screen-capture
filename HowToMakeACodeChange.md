### Guideline ###
Every code change should go through careful review before getting into the main branch.

### Post-commit code review ###
  * Branch the code in /trunk into /branches/your-new-branch.
  * Make changes in /branches/your-new-branch.
  * [Request code review](http://code.google.com/p/chrome-screen-capture/issues/entry?show=review&former=sourcelist)
  * Modify your code in /branches/your-new-branch based on your discussion with your code reviewer.
  * After you get a positive score from your code reviewer, merge /branches/your-new-branch back into /trunk.

Reference: [Basic SVN Commands](http://code.google.com/p/chrome-screen-capture/wiki/BasicSVNCommands)

### Pre-commit code review ###
  * Checkout the code in /trunk into a working copy, and make changes in your working copy.
  * Use gcl to generate a changelist and upload the changelist to [codereview.appspot.com](http://codereview.appspot.com/).
  * Find a code reviewer to review you code on [codereview.appspot.com](http://codereview.appspot.com/).
  * Modify your code and use gcl to upload it again when necessary.
  * After you get a positive score from your code reviewer, checkin your code to /trunk.

Reference: [Basic gcl Usage](http://code.google.com/p/chrome-screen-capture/wiki/BasicGclUsage)