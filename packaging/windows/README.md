# Creating a Windows installer

The scripts here help with cross building Subsurface and smtk2ssrf for Windows.

The preferred method to create a Windows installer is to use our own docker
image that has all the build components pre-assembled.
All it takes is this:

```
cd /some/path/windows
git clone https://github.com/subsurface/subsurface
cd subsurface
git submodule init
git submodule update
docker run -v /some/path/windows:/__w subsurface/mxe-build:3.1.0 /bin/bash /__w/subsurface/packaging/windows/create-win-installer.sh
```

This will result in subsurface-VERSION.exe and smtk2ssrf-VERSION.exe to be created.
