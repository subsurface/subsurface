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

## Windows installer running-process regression checklist

Run on a disposable Windows VM with a test logbook and a staged installer built
by `ninja installer` (MSVC) or the legacy MXE container build above. Record the
Windows version, NSIS version (`makensis -VERSION`), Subsurface version, build
variant, and the installer filename. For MSVC builds, retain the existing
`validate_windows_msvc_stage` check before invoking NSIS; a script-only NSIS
compile does not validate the staged Qt Location/Positioning payload.

1. Install a prior build to a disposable directory. Record hashes of its
   `subsurface.exe` and DLLs, for example with PowerShell
   `Get-FileHash 'C:\test\Subsurface\subsurface.exe'` and
   `Get-FileHash 'C:\test\Subsurface\*.dll'`.
2. Start that installed `subsurface.exe` with the test logbook open (also try
   minimised). Run the new installer interactively to the same directory.
   Verify the Save/Close and Retry/Cancel prompt appears before any installed
   executable or DLL hash changes. Retry while it is still running: it must
   prompt again. Cancel: hashes must remain unchanged and the application must
   still be running with its unsaved work untouched.
3. Start two `subsurface.exe` processes. Close one and Retry: still blocked.
   Save and close the other and Retry: installation succeeds. Also test a
   running `subsurface.exe` from a different installation path: it deliberately
   blocks this installer too, since the check uses process name, not path.
4. Run a similarly named process (such as `subsurface-test.exe`), but no
   `subsurface.exe`. Verify a fresh install and an upgrade proceed without
   this prompt. Use only disposable paths, not the real logbook.
5. With `subsurface.exe` running, run `.\subsurface-VERSION.exe /S /D=C:\test\Subsurface`
   from PowerShell, then inspect `$LASTEXITCODE`. It must return non-zero
   promptly without a dialog; existing hashes must not change. Repeat with
   no matching process: silent installation should complete normally.
6. To test enumeration failure, use an isolated instrumented VM/test build
   that makes `CreateToolhelp32Snapshot` or `Process32FirstW/NextW` fail with
   an error other than `ERROR_NO_MORE_FILES`. Confirm interactive Retry/Cancel
   and silent non-zero exit without changing payload. Do not induce failure by
   altering a production machine or disabling Windows protections.
7. If Subsurface starts after the snapshot but before extraction, the
   preflight cannot prevent the race. Check that ordinary NSIS file-error
   handling remains available rather than assuming the process guard covers
   every file lock.
