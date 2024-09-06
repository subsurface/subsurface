# Coding Style

Here are some of the basics that we are trying to enforce for our coding style
and conventions. The existing code (as of the commit that adds these lines) is
not yet fully consistent to these rules, but following these rules will make
sure that no one yells at you about your patches.

We have a script that can be used to reformat code to be reasonably close
to these rules; it's in `scripts/whitespace.pl` – this script requires
clang-format to be installed (which sadly isn't installed by default on
any of our platforms; even on Mac where clang is the default compiler).

At the end of this file are some ideas for your `.emacs` file (if that's
your editor of choice) as well as for QtCreator. If you have settings for
other editors that implement this coding style, please add them here.

## Basic style rules

* all indentation is tabs (set to 8 char) with the exception of
  continuation lines that are aligned with tabs and then spaces

* all keywords followed by a '(' have a space in between
  ```
  if (condition)
  
  for (i = 0; i < 5; i++)
  ```

* function calls do NOT have a space between their name and argument
  ```
  i = some_function(argument);
  ```

* usually there is no space on the inside of parenthesis (see examples
  above)

* function / method implementations have their opening curly braces in
  column 1

* all other opening curly braces follow at the end of the line, with a
  space separating them:
  ```
  if (condition) {
  	dosomething();
  	dosomethingelse();
  }
  ```

* both sides of an `if` / `else` clause either use or do not use curly braces:
  ```
  if (condition)
  	i = 4;
  else
  	j = 6;
  
  if (condition) {
  	i = 6;
  } else {
  	i = 4;
  	j = 6;
  }
  ```

* use space to make visual separation easier
  ```
  a = b + 3 + e / 4;
  ```

* continuation lines have the operator / comma at the end
  ```
  if (very_long_condition_1 ||
      condition_2)
  
  b = a + (c + d +
  	 f + z);
  ```

* in a C++ constructor initialization list, the colon is on the same line and
  continuation lines are aligned as the rule above:
  ```
  ClassName::ClassName() : x(1),
  	y(2),
  	z(3)
  {
  }
  ```

* unfortunate inconsistency

  - Core code usually uses underscores to structure names
    ```
    variable_or_class_in_core
    ```
  - In contrast, Qt / display layer code usually uses camelCase
    ```
    variableInQt
    ```
    for variable names and PascalCase
    ```
    ClassInQt
    ```
    for names of classes and other types

  where the two meet, use your best judgment and go for best consistency
  (i.e., where does the name "originate")

* macro names should be capitalized; e.g., prefer
  ```
  #define FROB 17
  #define FROBULATE(x) (x)+FROB
  ```
  to
  ```
  #define frob 17
  #define frobulate(x) (x)+frob
  ```

  Since C++ is strongly typed, avoid macros where possible.
  For constants use `constexpr`:
  ```
  static constexpr int frob = 17;
  ```
  and for functions use templated inline functions such as
  ```
  template<typename T>
  static bool less_than(T x, T y)
  {
    return x < y;
  }
  ```

* there is a strong preference for lower case file names; sometimes conventions
  or outside requirements make camelCase filenames the better (or only) choice,
  but absent such an outside reason all file names should be lower case
  This rule applies to new file names, existing file names should not be
  changed without a reason.

* cpp/h file names should usually be identical to the class name but in lower
  case. Where it seems appropriate, multiple, closely related classes can be
  in a single file with a more generic name.

* `switch` statements with blocks are a little bit special (to avoid indenting
  too far)
  ```
  switch (foo) {
  case FIRST:
  	whatever();
  	break;
  case SECOND: {
  	int i;
  	for (i = 0; i < 5; i++)
  		do_something(i);
  }
  }
  ```

## Separation of core and UI layer and historical remarks

Ideally, we strive for a separation of core functionality and UI layer.
In practice however, the distinction is rather fuzzy and the code base is
inconsistent. The current state is due to the fact that the project was
originally written in C with the gtk library. Later, the UI layer was
converted to Qt, whereas the core functionality was still C. Gradually
more and more Qt and C++ creeped into the core layer. Recently we
switched to full C++.

To keep the option of non-Qt frontends, we should strive to use as few Qt
primitives in the core code as possible. However, some parts
are deeply interwoven with Qt, such as for example the translation machinery.
Moreover, some platform independent features, such as regexps or URL handling
might be hard to replace.

## C++

Since the project was originally written in C, some of the creators and
original contributors may feel overwhelmed by all too
"modern" C++, so try to avoid "fancy" constructs such as template meta
programming, unless they make the code distinctly simpler.

Also many of the (potential) contributors will not have an extensive
background in C++, so strive for simplicity.

## Coding conventions

* variable declarations

  In C++ the lifetime of a variable often coincides with the
  lifetime of a resource (e.g. file) and therefore the variable is defined
  at the place where the resource is acquired. The resource is freed,
  when the variable goes out of scope.

* The `*`, `&` and `&&` declarators are grouped with the name, not the type
  (classical C-style) as in `char *string` instead of `char* string`. This
  reflects the precedence rules of the language: `int &i` means that the name
  `i` stands for a reference [to an object with type `int`], not that
  `i` stands for an object of the type [reference to `int`].
  Although this may seem like hairsplitting (both interpretations
  have the same effect) it is crucial in the
  definition of multiple variables, such
  as
  ```
  struct dive *next, **pprev;
  ```

* We generally use explicit types in variable declarations for clarity.
  Use `auto` sparingly and only in cases where code readability improves.
  Two classical examples are:
  - Iterators, whose type names often are verbose:
    ```
    auto it = m_trackers.find(when);
    ```
    is not only distinctly shorter than
    ```
    std::map<qint64, gpsTracker>::iterator it = m_trackers.find(when);
    ```
    it will also continue working if a different data structure is chosen.
  - If the type is given in the same line anyway. Thus,
    ```
    auto service = std::make_unique<QLowEnergyService*>(sender());
    ```
    is easier to read than and conveys the same information as
    ```
    std::unique_ptr<QLowEnergyService> service = std::make_unique<QLowEnergyService>(sender());
    ```

* containers

  The standard library (STL) containers are robust, but their usage may
  appear verbose. Therefore, we have a few convenience functions in the
  `core/ranges.h` header.
  For example, to loop with an index variable, use
  ```
  for (auto [idx, v]: container) {
    ...
  }
  ```

* text strings

  The default language of subsurface is US English so please use US English
  spelling and terminology.
  User-visible strings should be passed to the `tr()` function to enable
  translation into other languages.
  - like this
    ```
    QString msgTitle = tr("Check for updates.");
    ```
  - rather than
    ```
    QString msgTitle = "Check for updates.";
    ```

  This works by default in classes (indirectly) derived from `QObject`. Each
  string to be translated is associated with a context, which corresponds
  to the class name. Classes that are not derived from `QObject` can generate
  the `tr()` functions by using the `Q_DECLARE_TR_FUNCTIONS` macro:
  ```
  #include <QCoreApplication>
  
  class myClass {
  	Q_DECLARE_TR_FUNCTIONS(gettextfromC)
  	...
  };
  ```

  As an alternative, which also works outside of class context, the `tr()`
  function of a different class can be called. This avoids creating multiple
  translations for the same string:
  ```
  gettextFromC::tr("%1km")
  ```

  The `gettextFromC` class in the above example was created as a catch-all
  context for translations accessed in core code. To use it from C, include
  the `"core/gettext.h"` header and invoke the `translate()` macro:
  ```
  #include "core/gettext.h"
  
  report_error(translate("gettextFromC", "Remote storage and local data diverged"));
  ```
  It is crucial to pass `"gettextFromC"` as a first macro argument so that Qt
  is able to associate the string with the correct context.
  The translate macro returns a cached C-style string, which is generated at runtime
  when the particular translation string is encountered for the first time.
  It remains valid during the whole application's life time.

  Outside of function context, the `QT_TRANSLATE_NOOP` macro can be used as in
  ```
  struct ws_info_t ws_info[100] = {
  	{ QT_TRANSLATE_NOOP("gettextFromC", "integrated"), 0 },
  	{ QT_TRANSLATE_NOOP("gettextFromC", "belt"), 0 },
  	{ QT_TRANSLATE_NOOP("gettextFromC", "ankle"), 0 },
  	{ QT_TRANSLATE_NOOP("gettextFromC", "backplate"), 0 },
  	{ QT_TRANSLATE_NOOP("gettextFromC", "clip-on"), 0 },
  };
  ```
  Note that here, the texts will be scheduled for translation with the `"gettextFromC"`
  context, but the array is only initialized with the original text. The actual
  translation has to be performed later in code. For C-code, the `QT_TRANSLATE_NOOP`
  macro is defined in the `"core/gettext.h"` header.

* UI text style

  These guidelines are designed to ensure consistency in presentation within
  Subsurface.
  Only the first word of multi-word text strings should be capitalized unless
  a word would normally be capitalized mid-sentence, like Africa. This applies
  to all UI text including menus, menu items, tool-tips, button text and label
  text etc. e.g. "Check for updates" rather than "Check for Updates".
  We also capitalize Subsurface (NOTE: not SubSurface) when referring to the
  application itself.
  Abbreviations should end with a period, e.g. "temp." not "temp" for
  temperature
  Numerals in chemical formulae should use subscript characters e.g. O₂ not O2
  Partial pressures in Subsurface are, by convention, abbreviated with a single
  "p" rather than 2, as in pO₂ not ppO₂
  Where more than one term exists for something, please choose the one already
  in use within Subsurface e.g. Cylinder vs. Tank.


* string manipulation

 - user interface

    In UI part of the code use of `QString` methods is preferred, see this pretty
    good guide in [`QString` documentation][1]

 - core components

    In the core part of the code, std::string should be used.

* memory management in core

  In core code, objects are typically stored in containers, such as `std::vector<>` or
  as subobjects of classes.

  If an object has to be allocated on the heap, the owner keeps an `std::unique_ptr`.
  To transfer ownership, use `std::move()`.

* initialization and life time

  By using subobjects, the life time of objects is well defined.
  Consider a class A1 with the two subobjects B and C:
  ```
  class A1 {
    struct B;
    struct C;
  };
  ```
  furthermode, consider a class A2 derived from A1 with the subobjects D and E:
  ```
  class A2 : public A1 {
    struct D;
    struct E;
  };
  ```
  When creating an object of type A2, the constructors are run in the following order:
    - B
    - C
    - A1
    - D
    - E
    - A2
  The destructors run in opposite order.
  This means that C can *always* access B, but not vice-versa and so on.

  Subobjects should be initialized using initializer lists, so that they are initoalized
  only once.
  
* pointers and references

  The difference between pointers and references can be confusing to C programmers,
  as internally they are realized by the same mechanism. However, conceptually they
  are different: a reference is a placeholder for a variable.

  In particular this means two things:
    - A reference cannot be 'reseated'. It stands for a different variable and only
      that variable. There is no pointer arithmetic with references.
    - A reference cannot be null. In fact any reasonable compiler will compile
      ```
        void f(int &f) {
            return &f == nullptr ? 1 : 2;
        }
      ```
      as
      ```
        f(int&):
            mov     eax, 2
            ret
      ```

  Thus, functions should in general take references, not pointers. A pointer argument is
  basically only used if the argument is optional.

* output parameters

  If a function returns multiple values, generally don't return them in output parameters,
  but return a structure of multiple values. This can be used in structured bindings:
  ```
    [val, i] = get_result();
  ```

## Sample Settings

### Emacs

These lines in your `.emacs` file should get you fairly close when it comes
to indentation – many of the other rules you have to follow manually

```
;; indentation
(defun c-lineup-arglist-tabs-only (ignored)
  "Line up argument lists by tabs, not spaces"
  (let* ((anchor (c-langelem-pos c-syntactic-element))
         (column (c-langelem-2nd-pos c-syntactic-element))
         (offset (- (1+ column) anchor))
         (steps (floor offset c-basic-offset)))
    (* (max steps 1)
       c-basic-offset)))

(add-hook 'c-mode-common-hook
          (lambda ()
            ;; Add kernel style
            (c-add-style
             "linux-tabs-only"
             '("linux" (c-offsets-alist
                        (arglist-cont-nonempty
                         c-lineup-gcc-asm-reg
                         c-lineup-arglist-tabs-only))))))

(add-hook 'c-mode-hook
          (lambda ()
            (let ((filename (buffer-file-name)))
              ;; Enable kernel mode for the appropriate files
                (setq indent-tabs-mode t)
                (c-set-style "linux-tabs-only"))))

(add-hook 'c++-mode-hook
          (lambda ()
            (let ((filename (buffer-file-name)))
              ;; Enable kernel mode for the appropriate files
                (setq indent-tabs-mode t)
                (c-set-style "linux-tabs-only"))))
```

### QtCreator

These settings seem to get indentation right in QtCreator. Making TAB
always adjust indent makes it hard to add hard tabs before '\' when
creating continuing lines. Copying a tab with your mouse / ctrl-C and
inserting it with ctrl-V seems to work around that problem (use Command
instead of ctrl on your Mac)
Save this XML code below to a file, open Preferences (or Tools->Options)
in QtCreator, pick C++ in the left column and then click on Import...
to open the file you just created. Now you should have a "Subsurface"
style that you can select which should work well for our coding style.

```
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE QtCreatorCodeStyle>
<!-- Written by QtCreator 3.0.0, 2014-02-27T07:52:57. -->
<qtcreator>
 <data>
  <variable>CodeStyleData</variable>
  <valuemap type="QVariantMap">
   <value type="bool" key="AlignAssignments">false</value>
   <value type="bool" key="AutoSpacesForTabs">false</value>
   <value type="bool" key="BindStarToIdentifier">true</value>
   <value type="bool" key="BindStarToLeftSpecifier">false</value>
   <value type="bool" key="BindStarToRightSpecifier">false</value>
   <value type="bool" key="BindStarToTypeName">false</value>
   <value type="bool" key="ExtraPaddingForConditionsIfConfusingAlign">false</value>
   <value type="bool" key="IndentAccessSpecifiers">false</value>
   <value type="bool" key="IndentBlockBody">true</value>
   <value type="bool" key="IndentBlockBraces">false</value>
   <value type="bool" key="IndentBlocksRelativeToSwitchLabels">false</value>
   <value type="bool" key="IndentClassBraces">false</value>
   <value type="bool" key="IndentControlFlowRelativeToSwitchLabels">true</value>
   <value type="bool" key="IndentDeclarationsRelativeToAccessSpecifiers">true</value>
   <value type="bool" key="IndentEnumBraces">false</value>
   <value type="bool" key="IndentFunctionBody">true</value>
   <value type="bool" key="IndentFunctionBraces">false</value>
   <value type="bool" key="IndentNamespaceBody">false</value>
   <value type="bool" key="IndentNamespaceBraces">false</value>
   <value type="int" key="IndentSize">8</value>
   <value type="bool" key="IndentStatementsRelativeToSwitchLabels">true</value>
   <value type="bool" key="IndentSwitchLabels">false</value>
   <value type="int" key="PaddingMode">2</value>
   <value type="bool" key="SpacesForTabs">false</value>
   <value type="int" key="TabSize">8</value>
  </valuemap>
 </data>
 <data>
  <variable>DisplayName</variable>
  <value type="QString">Subsurface</value>
 </data>
</qtcreator>
```

### Vim

As everybody knows vim is a way better editor than emacs and thus needs to be
in this file too. Put this into your `.vimrc` and this should produce something
close to our coding standards.

```
" Subsurface coding style
filetype plugin indent on
filetype detect
set cindent tabstop=8 shiftwidth=8 cinoptions=l1,:0,(0,g0

" And some sane defaults, optional, but quite nice
set nocompatible
syntax on
colorscheme default
set hls
set is

" The default blue is just impossible to see on a black terminal
highlight Comment ctermfg=Brown

" clearly point out when someone have trailing spaces
highlight ExtraWhitespace ctermbg=red guibg=red

" Show trailing whitespace and spaces before a tab:
match ExtraWhitespace /\s\+$\| \+\ze\t/
```

[1]: http://doc.qt.io/qt-5/qstring.html#manipulating-string-data
[2]: https://github.com/Subsurface/subsurface/blob/master/core/membuffer.h
