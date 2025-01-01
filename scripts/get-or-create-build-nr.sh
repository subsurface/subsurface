#!/bin/bash
# this is comically complicated - why does GitHub not have a monotonic number
# that tracks how many times any action was kicked off? Or an atomic way to update a variable?
# So we use the fact that git itself does a great job of preventing us from overwriting an
# existing branch and abuse that to create atomicity

SHA_BRANCH="branch-for-$1"

# first - make sure git is configured so we can do stuff
git config --global user.email "ci@subsurface-divelog.org"
git config --global user.name "Subsurface CI"

# next, clone the release repo
[ -d nightly-builds ] || git clone https://github.com/subsurface/nightly-builds
cd nightly-builds

# this is from the main branch, so this should be the PREVIOUS build number
latest=$(<latest-subsurface-buildnumber)

# now let's see if a branch for the current SHA exists
if git switch "$SHA_BRANCH"
then
  # one of the other workflows created a release number already
  latest=$(<latest-subsurface-buildnumber)
else
  # this is almost certainly a race between the different workflow files
  # the main branch should have held the previous release number
  # increment by one and write as new build number into the named branch
  latest=$((latest+1))
  echo "new build number is $latest"

  git switch -c "$SHA_BRANCH"
  echo $latest > latest-subsurface-buildnumber
  git commit -a -m "record build number for this SHA"

  # now comes the moment of truth - are we the first one?
  # the push will succeed for exactly one of the workflows
  if git push origin "$SHA_BRANCH"
  then
    # yay - we win! now let's make sure that we remember this number for next time
    git switch main
    echo $latest > latest-subsurface-buildnumber
    git commit -a -m "record latest build number in main branch"
    if ! git push origin main
    then
      echo "push to main failed - we'll lose monotonic property"
      exit 1
    fi
  else
    # someone else was faster - get the number they wrote
    git switch main
    git branch -D "$SHA_BRANCH"
    git pull
    if ! git switch "$SHA_BRANCH"
    then
      echo "push to $SHA_BRANCH failed, but switching to it failed as well"
      exit 2
    fi
    latest=$(<latest-subsurface-buildnumber)
  fi
fi
# if we get here, we have the next build number - wow that was weird.
echo "Build number is $latest"
