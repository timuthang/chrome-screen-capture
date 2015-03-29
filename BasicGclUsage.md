### Install gcl ###
gcl is a code review tool for subversion. It's a part of Chromium's [depot\_tools](http://dev.chromium.org/developers/how-tos/depottools). You can follow the install instructions [here](http://dev.chromium.org/developers/how-tos/install-gclient).

### Use gcl to send code reviews ###
  1. Checkout /trunk into a working copy
> > `svn checkout https://chrome-screen-capture.googlecode.com/svn/trunk chrome-screen-capture`
  1. Make code change and generate a changelist (record the changelist id, referred as `A1B2` below)
> > `gcl change`
  1. Upload the changelist to codereview.appspot.com (record the issue id, referred as `1234567` below)
> > `gcl upload A1B2 -s codereview.appspot.com -r reviewer@gmail.com -cc extension-code-commits@googlegroups.com -m "changelist description" --send_mail`
  1. Make more code change and update the changelist if necessary
> > `gcl change A1B2`
  1. Upload the changelist again to the same issue in codereview.appspot.com
> > `gcl upload A1B2 -s codereview.appspot.com -i 1234567 --send_mail`
  1. Commit the changelist
> > `gcl commit A1B2`