# Instructions for building the container environment and using it to build/package subsurface.

This document assumes you have alreay installed docker and have checked out subsurface according to the instructions in the INSTALL document.

If you just want to build with the current mxe build container then starting from the folder above subsurface run

```bash
docker run -v $PWD/win32:/win/win32 -v $PWD/subsurface:/win/subsurface --name=mybuilder -w /win -d subsurface/mxe-build:x.y /bin/sleep 60m
```

replacing the x.y in the mxe-build tag with the current version e.g. 3.1.0

Next you need to prep the container by installing some prerequisites

```bash
docker exec -t mybuilder bash subsurface/.github/workflows/scripts/windows-container-prep.sh 2>&1 | tee pre-build.log
```

Finaly the actual build is done with
```bash
docker exec -t mybuilder bash subsurface/.github/workflows/scripts/windows-in-container-build.sh 2>&1 | tee build.log
```

To get the built binary out of the container
```
docker exec -t mybuilder bash -c "cp /subsurface-*-installer.exe /win/win32"
```
Which will copy the installer into the win32 folder which will be a sibling of the subsurface folder.

## Modifying the container
If you want to make changes to the build environment used in the conatiner
The script scripts/docker/mxe-build-container/build-container.sh will build the Docker image itself.
The sha of the version of MXE we are using is built into this, so you can update that to whatever version is required, and modify dockerfiles and settings-stage1.mk and settings-stage2.mk to pull in any other prerequisites as required.

If you are working on updating the container then you should incrment the minor version of the variable VERSION in the script as otherwise it will clash with the version used in production

