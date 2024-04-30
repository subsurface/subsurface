# Creating a Windows installer

The scripts here help with cross building Subsurface and smtk2ssrf for Windows.

The preferred method to create a Windows installer is to use our own docker
image that has all the build components pre-assembled.
All it takes is this:

```.sh
export GIT_AUTHOR_NAME=<your name>
export GIT_AUTHOR_EMAIL=<email to be used with github>

cd /some/path
git clone https://github.com/subsurface/subsurface
cd subsurface
git submodule init
git submodule update
./packaging/windows/docker-build.sh
```

This will result in the two installers subsurface-VERSION.exe and smtk2ssrf-VERSION.exe to be created in /some/path/subsurface/output/windows/.
