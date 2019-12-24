Testing subsurface:

Subsurface contains a number of tests to ensure the stability of the product, these tests can be used
manually as they are in the build process.

Making the tests available is automatic, whenever building either desktop or mobile (for desktop) 
the tests are created.

The source code for the tests are found in
- subsurface/tests
and the executables are (after creation) found in
- subsurface/build/tests
or 
- subsurface/build-mobile/tests

To run a specific test do as in this example:
cd subsurface/<build directory>/tests
./TestTagList

To run the whole suite do:
cd subsurface/<build directory>/tests
ctest

To get more verbose (a lot more) do:
cd subsurface/<build directory>/tests
ctest -V

hint try "man ctest" or "ctest --help"

If you have multiple versions of Qt installed,
you might get a "plugin missing error", you can fix that by doing

export QT_QPA_PLATFORM_PLUGIN_PATH=~/Qt/5.13.2/clang_64/plugins
(of course substitute 5.13.2 with your preferred version)

We need more tests so please have fun.

if you just want to create a new test case:

1) Create a new private slot on an already created test class
2) Implement the test there, and compare the expected result with the
correct result with QCOMPARE:

void testRedCeiling();
testRedCeiling()
{
	load_dive("../dive/testDive.xml");
	calculated_ceiling();
	QCOMPARE( dive->ceiling->color(), QColor("red"));
}

3) build the system
4) Run the test case and see result

if you want to create a new test

1) amend subsurface/tests/CMakeLists.txt 
1.1) there are 3 places where the test needs to be added
1.1.1) simplest way is to search for e.g. TestPlannerShared and add your test in a similar way
2) Create the source files (.h and .cpp all lowercase)
2.1) simplest way is to copy one of the other test sources
3) build system
4) correct any errors
5) run test
5.1) subsurface/<build directory>/tests/Test<name>
6) Fix the test case

If the color is not QColor("red"), when you run the test you will get a
failure. Then you run a command to get a more verbose output and see in
which part the test fails.

$ ctest -V

7) submit PR to get the test merged into the product.

Hint look at the existing test cases if you run into trouble or need more
ideas. E.g. running following command will show the first test cases
written for unit conversions:

Also the Qt documentation is good source for more information:

http://qt-project.org/doc/qt-5/qtest.html

There are three main test macros that we use, but of course many more
are available (check the Qt documentation):

QCOMPARE(value, expected)
QVERIFY(boolean)
QVERIFY2(boolean, "error message on fail")

If expecting a test case to fail, use

QEXPECT_FAIL("", "This test case fails, as it is not yet fully implemented", Continue);
