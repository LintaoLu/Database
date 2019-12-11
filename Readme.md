## Building and running
To compile and run the project do:
>mkdir build
>
>cd build
>
>cmake ..
>
>make -jx (where x is a number of threads)
>
>make test

The last command will run all of the tests defined.

You can also find the individual test scripts executables in ```build/test```

## New Tests
To add a new test, simply go to test/src and put you're new test, a cpp file,
 into the corresponding folder, so if it is for the first assigment, it should go inside
 test/src/databaseManager, we will add new folders during the next assigments.
 
Once the cpp is there, go to test/CMakeList.txt and add:
>set(@ASSIGMENTNAME_SRC_@TESTNAME_TEST src/databaseManager/@FILE.c)
>
>add_executable (@testname ${COMMON_TEST_SRC} ${@ASSIGMENTNAME_SRC_@TESTNAME_TEST})
>
>target_link_libraries (@testname -lcommon -ldatabaseManager)
>
>add_test (NAME @testname COMMAND @testname)
>

Replace the variables, marked as @xxxx (including the "@"), with the respective values. For example, if we create
a new test for assigment 1, the databaseManager that tests, lets say, network issues 
(a stupid example no need for this obviously), we should use:
@ASSIGMENTNAME = DATABASEMANAGER\
@TESTNAME = NETWORK \
@FILE = network_issues\
@testname = networktest

It will end up something like:
>set(DATABASEMANAGER_SRC_NETWORK_TEST src/databaseManager/network_issues.c)
>
>add_executable (networktest ${COMMON_TEST_SRC} ${DATABASEMANAGER_SRC_NETWORK_TEST})
>
>target_link_libraries (networktest -lcommon -ldatabaseManager)
>
>add_test (NAME networktest COMMAND networktest)
>


## Adding new projects
For the next assignment.
