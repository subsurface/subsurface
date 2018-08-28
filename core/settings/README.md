# Short explanation of how qPref works

## System startup:

### core/qt-init.cpp init_qt_late()
does
```
qPref::load();
```
to load all properties from all qPref* classes

## System shutdown:

### subsurface-mobile-main.cpp and subsurface-desktop-main.cpp main()
calls
```
call qPref::sync()
```
to save all changes to disk, this is needed because part of the program
modifies struct preferences instead of calling the set method.

#### EXCEPTION:
Variables not present in struct preferences (local static variables in the class
are not written, see later.

## System startup/shutdown common handling:
```
qPref::load() calls qPref::doSync(false)
qPref::sync() calls qPref::doSync(true)
```

### qPrefDoSync()
qPref::doSync() calls qPref<xyz>::doSync() for all qPref* classes
Meaning qPref knows which classes exist, but not which variables

### qPref*::doSync() calls
```
disk_<variable name>() for each variable
```

#### EXCEPTION: 
some variables are not in struct preferences, but local static variables
which can only be accessed through the set/get functions, therefore there
are no need to sync them
```
	if (!doSync) // meaining load
		load_<variable_name>()
```

### qPref*::disk_*()
qPref*::disk_*() calls qPrefPrivate::propSetValue to save a property, 
and qPrefPrivate::propValue() to read a property. The function also
keeps struct preferences in sync.

### qPrefPrivate::propSetValue()
qPrefPrivate::propSetValue() is static and calls QSettings to write property

### qPrefPrivate::propValue()
qPrefPrivate::propValue() is static and calls QSettings to read property

### macros 
the macros are defined in qPrefPrivate.h

the macros are used in qPref*cpp 

## Reading properties
Reading a property is done through an inline function, and thus no more expensive
than accessing the variable directly.

```
qPref*::<variable_name>()
```

## Setting properties
Setting a property is done through a dedicated set function which are static to
simplify the call:
```
qPref<class>::set_<variable_name>(value)
```
like:
```
qPrefDisplay::animation_speed(500);
```

the set function updates struct preferences (if not a local variable), and saves
the property to disk, however save is only done if the variable is changed.

##  Updating struct preferences 
When a program part updates struct preferences directly, changes are NOT saved to disk
if the programm crashes, otherwise all variables are saved to disk with the program exists.

