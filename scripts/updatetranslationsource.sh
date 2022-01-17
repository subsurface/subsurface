#!/bin/bash
#
# ugly hack - this does NOT work with the normal layout of our sources
# the build dir needs to be outside of the src/subsurface tree for this to work
# in order to run this I have this layout
# .../src/subsurface
# .../src/subsurface-build
# and then run this in that latter directory

if [[ ! -d translations || ! -f translations/subsurface_source.qm ]] ; then
	echo Start from the build folder
	exit 1
fi

if [[ "$1" = "-nopush" ]] ; then
	NOPUSH="1"
fi

SRC=$(grep Subsurface_SOURCE_DIR CMakeCache.txt | cut -d= -f2)

pushd $SRC

# let's make sure the tree is clean
git status | grep "Changes not staged for commit" 2>/dev/null && echo "tree not clean" && exit 1
git status | grep "Changes to be committed" 2>/dev/null && echo "tree not clean" && exit 1

# now remove the translations and remove access to the kirigami sources
# and any old sources under tmp as well as build directories inside the source tree
chmod 000 mobile-widgets/3rdparty
[ -d tmp ] && chmod 000 tmp
[ -d build ] && chmod 000 build
[ -d build-mobile ] && chmod 000 build-mobile
rm translations/subsurface_source.ts

# enable creating the translation strings
sed -i.bak 's/# qt5_create_translation/ qt5_create_translation/ ; s/# add_custom_target(translations_update/ add_custom_target(translations_update/' translations/CMakeLists.txt

popd

# recreate make files and create translation strings
cmake .
pushd translations
make translations_update > translationsupdate.log 2>&1
popd

# restore the CMakeLists.txt and rebuild makefiles
cp $SRC/translations/CMakeLists.txt.bak $SRC/translations/CMakeLists.txt
cmake .

pushd $SRC

# double up the numerusform lines so Transifex is happy
awk '/<numerusform><\/numerusform>/{print $0}{print $0}' translations/subsurface_source.ts > translations/subsurface_source.ts.new
mv translations/subsurface_source.ts.new translations/subsurface_source.ts

# now add the new source strings to git and remove the rest of the files we created 
git add translations/subsurface_source.ts
git commit -s -m "Update translation source strings"
git reset --hard

# now enable access to kirigami again
chmod 755 mobile-widgets/3rdparty
[ -d tmp ] && chmod 755 tmp
[ -d build ] && chmod 755 build
[ -d build-mobile ] && chmod 755 build-mobile

# this really depends on my filesystem layout
# push sources to Transifex
if [[ "$NOPUSH" != "1" ]] ; then
	~/transifex-client/tx push -s
fi
