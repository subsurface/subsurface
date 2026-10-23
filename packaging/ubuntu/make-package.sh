#!/bin/bash -e
# start from the directory above the subsurface directory
#
# we need to build the google maps plugin which is not part of our sources, so let's make sure
# it is included in our source tar file
#

if [[ ! -d subsurface || ! -d subsurface/libdivecomputer ]] ; then
	echo "Please start this script from the folder ABOVE the subsurface source directory"
	exit 1;
fi
if [[ ! -d googlemaps/.git ]] ; then
	echo "Please make sure you have a git clone of https://github.com/Subsurface/googlemaps"
	echo "checked out in parallel to the Subsurface directory (we need both its master"
	echo "and its qt6-upstream branch)"
	exit 1;
fi
if [[ ! -d qlitehtml/.git ]] ; then
	echo "Please make sure you have a git clone of https://github.com/dirkhh/qlitehtml"
	echo "(with its litehtml submodule) checked out in parallel to the Subsurface directory"
	exit 1;
fi

# we need the remote tracking branches of googlemaps to be current
(cd googlemaps && git fetch origin)

# export a branch of googlemaps from the git repo into a directory
# (no fallback to HEAD - building the wrong branch only fails on Launchpad)
export_googlemaps() {
	local branch="$1" dest="$2"
	if ! git -C ../../googlemaps rev-parse --verify --quiet "origin/$branch" > /dev/null ; then
		echo "googlemaps branch origin/$branch not found"
		exit 1
	fi
	mkdir -p "$dest"
	git -C ../../googlemaps archive "origin/$branch" | tar xf - -C "$dest"
}

# The googlemaps plugin needs private QtLocation and QtPositioning headers that the
# Ubuntu Qt 6 packages don't ship, and Launchpad builds have no network access. So
# we vendor them into the source tree, one directory per Qt minor version (the
# newest patch release of each); debian/rules.qt6 picks the one matching the Qt on
# the build host. Add a minor version here when a supported Ubuntu release gets it.
QT6_PRIVATE_HEADER_MINORS="6.8 6.9 6.10 6.11"
export_qt_private_headers() {
	local minor tag tmp dest
	tmp=$(mktemp -d)
	for minor in $QT6_PRIVATE_HEADER_MINORS ; do
		tag=$(git ls-remote --tags --refs https://github.com/qt/qtlocation.git "v${minor}.*") || {
			echo "failed to query qtlocation releases for Qt $minor" >&2
			exit 1
		}
		tag=$(printf '%s\n' "$tag" | sed -e 's@.*refs/tags/@@' | grep -E '^v[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
		if [[ -z "$tag" ]] ; then
			echo "no qtlocation release found for Qt $minor - skipping"
			continue
		fi
		echo "getting the private QtLocation headers for $tag"
		if ! git clone --quiet -c advice.detachedHead=false --branch "$tag" --depth 1 https://github.com/qt/qtlocation.git "$tmp/qtlocation" ; then
			echo "failed to fetch private QtLocation headers for $tag" >&2
			rm -rf "$tmp"
			exit 1
		fi
		if ! git clone --quiet -c advice.detachedHead=false --branch "$tag" --depth 1 https://github.com/qt/qtpositioning.git "$tmp/qtpositioning" ; then
			echo "failed to fetch private QtPositioning headers for $tag" >&2
			rm -rf "$tmp"
			exit 1
		fi
		dest="$PWD/QtHeaders/$minor/include"
		mkdir -p "$dest/QtLocation/private" "$dest/QtPositioning/private"
		find "$tmp/qtlocation/src/location" -name '*_p.h' -exec cp {} "$dest/QtLocation/private/" \;
		find "$tmp/qtpositioning/src/positioning" -name '*_p.h' -exec cp {} "$dest/QtPositioning/private/" \;
		rm -rf "$tmp/qtlocation" "$tmp/qtpositioning"
	done
	rm -rf "$tmp"
	if [[ ! -d QtHeaders/6.10 ]] ; then
		echo "failed to get the private Qt headers"
		exit 1
	fi
}

# export the files tracked by qlitehtml (including its litehtml submodule) - this
# leaves out the artifacts of any in-source build in the checkout
export_qlitehtml() {
	mkdir -p qlitehtml
	git -C ../../qlitehtml ls-files -z --recurse-submodules | tar -C ../../qlitehtml --null -T - -cf - | tar xf - -C qlitehtml
}

# ensure that the libdivecomputer module is there and current
cd subsurface
git submodule init
git submodule update

BUILDNUMBER=$(bash scripts/get-version.sh 1)
GITVERSION=$(bash scripts/get-version.sh 4)
GITDATE=$(git log -1 --format="%at" | xargs -I{} date -d @{} +%Y-%m-%d)
LIBDCREVISION=$(cd libdivecomputer ; git rev-parse --verify HEAD)
FOLDER="subsurface_$GITVERSION"

# hardcode to 1 to mark that this is a test build, not a full release
GITREVISION=1

cd -

export DEBSIGN_PROGRAM="gpg --passphrase-file passphrase_file.txt --batch --no-use-agent"
export DEBSIGN_KEYID="F58109E3"

if [[ "$GITVERSION" = "" ]] ; then
	SUFFIX=".release"
else
	SUFFIX=".daily"
fi

echo "building Subsurface $GITVERSION with libdivecomputer $LIBDCREVISION"

# we put all of the files into the distrobuilds directory in order not to clutter the 'src' directory
mkdir -p distrobuilds
cd distrobuilds

if [[ ! -d $FOLDER ]]; then
	mkdir "$FOLDER"
	echo "copying sources"

	(cd ../subsurface ; tar cf - . ) | (cd "$FOLDER" ; tar xf - )
	cd "$FOLDER";

	export_googlemaps master googlemaps
	export_googlemaps qt6-upstream googlemaps-qt6
	export_qlitehtml
	export_qt_private_headers

	rm -rf .git libdivecomputer/.git build build-mobile libdivecomputer/build
	echo "$BUILDNUMBER" > latest-subsurface-buildnumber
	echo "$GITDATE" > .gitdate
	echo "$LIBDCREVISION" > libdivecomputer/revision

	echo "creating source tar file for Ubuntu PPA"

	tar cf - . | xz > ../"$FOLDER.orig.tar.xz"
else
	echo "using existing source tree"
	cd "$FOLDER"
	[[ -d googlemaps-qt6 ]] || export_googlemaps qt6-upstream googlemaps-qt6
	[[ -d qlitehtml ]] || export_qlitehtml
	[[ -d QtHeaders ]] || export_qt_private_headers
fi


echo "preparing the debian directory"

# this uses my (Dirk's) email address by default... simply set that variable in your
# environment prior to starting the script
[[ -z $DEBEMAIL ]] && export DEBEMAIL=dirk@subsurface-divelog.org

# copy over the debian files and allow maintaining a release and daily changelog files
rm -rf debian
mkdir -p debian
cp -a ../../subsurface/packaging/ubuntu/debian .
rm -f debian/control.qt6 debian/rules.qt6

# start with the bundled dummy changelog, but use our last one if it exists
[[ -f ../changelog$SUFFIX ]] && cp "../changelog$SUFFIX" debian/changelog

# pick a new revision number
rev=0
while [ $rev -le "99" ]
do
rev=$((rev+1))
	if ! grep -q "$GITVERSION-$rev" debian/changelog ; then
		break
	fi
done

# we need to do this for each Ubuntu release we support - right now the oldest is 20.04/Focal
if [[ "$GITREVISION" = "" ]] ; then
	dch -v "$GITVERSION-$rev~focal" -D focal -M -m "Next release build - please check https://subsurface-divelog.org/category/news/ for details"
else
	dch -v "$GITVERSION-$rev~focal" -D focal -M -m "next rolling CICD release build"
fi

cp debian/changelog ../changelog$SUFFIX
cp debian/control ../control$SUFFIX

debuild -S -d

# create builds for the newer Ubuntu releases that Launchpad supports
#
rel=focal
others="jammy noble"
others_qt6="resolute stonking"

build() {
	local next="$1"

	sed -i "s/${rel}/${next}/g" debian/changelog
	debuild -S -d
	rel=$next
}

# older ubuntu releases use Qt5
for next in $others
do
	build "$next"
done

# Ubuntu 26.04 and newer use Qt6
cp ../../subsurface/packaging/ubuntu/debian/control.qt6 debian/control
cp ../../subsurface/packaging/ubuntu/debian/rules.qt6 debian/rules

for next in $others_qt6
do
	build "$next"
done

cd ..

if [[ "$1" = "current" ]] ; then
	PPA="ppa:subsurface/subsurface"
else
	PPA="ppa:subsurface/subsurface-daily"
fi

# Upload each distro variant separately so that orig.tar.xz is included in
# every batch.
# Launchpad's FTP upload endpoint randomly returns transient 550 errors
# ("Requested action not taken: internal server error"), so retry each
# upload a few times with linear backoff before giving up. After all
# uploads have been attempted, error out at the end if one or more
# definitively failed.
failed=""
max_attempts=5
for f in "${FOLDER}-${rev}"~*.changes; do
	attempt=1
	while : ; do
		rm -f ~/.dput.log
		dput_output=$(dput "$PPA" "$f" 2>&1) && dput_status=0 || dput_status=$?
		echo "$dput_output"
		if [ $dput_status -eq 0 ]; then
			break
		fi
		# Permanent Launchpad rejections (file already uploaded) are not
		# transient; retrying will not help and wastes all remaining attempts.
		if echo "$dput_output" | grep -qiE "File already exists|already exists in Primary|rejected"; then
			echo "WARNING: dput permanently rejected $f (already uploaded or rejected) - skipping retries"
			failed="$failed $f"
			break
		fi
		if [ "$attempt" -ge "$max_attempts" ]; then
			echo "WARNING: dput failed for $f after $attempt attempts"
			failed="$failed $f"
			break
		fi
		delay=$((attempt * 30))
		echo "dput failed for $f (attempt $attempt/$max_attempts), retrying in ${delay}s..."
		sleep "$delay"
		attempt=$((attempt + 1))
	done
done
if [ -n "$failed" ]; then
	echo "The following uploads failed:$failed"
	exit 1
fi
