## Testing openmonero

[Googletest](https://github.com/google/googletest) and
[googlemock](https://github.com/google/googletest/tree/master/googlemock)
frameworks are used for unit testing of openmonero. There is no need
to install them, as they are provided with openmonero.

### MySQL setup

Testing openmonero's operations on MySQL database are performed using
actuall mysql database. For this purpose a testing database with
some pre-populated data is used: `openmonero_test`. Thus before
tests are run, make sure that `openmonero_test` database is initilized
using `openmonero/sql/openmonero_test.sql` file, and `database_test` contains
information on conneting to the test database in
`openmonero/config/config.json`.

### Compile and run openmonero  tests

```bash
# go into build folder of openmonero
cd openmonero/build

# indicate that test should be build
cmake -DBUILD_TESTS=ON ..

# compile openmonero with tests
make

# run all tests
make test

# the above command will produce summary of test results (shown below).
# for verbose output, the following command can use used
# make test ARGS=-V

# individual tests executables can also be run. they are located in
# openmonero/build/tests

```

Example test output is:

```bash
mwo@arch:build$ make test
Running tests...
Test project /home/mwo/openmonero/build
    Start 1: mysql_tests
1/4 Test #1: mysql_tests ......................   Passed   60.64 sec
    Start 2: microcore_tests
2/4 Test #2: microcore_tests ..................   Passed    1.67 sec
    Start 3: bcstatus_tests
3/4 Test #3: bcstatus_tests ...................   Passed   25.39 sec
    Start 4: txsearch_tests
4/4 Test #4: txsearch_tests ...................   Passed    1.04 sec

100% tests passed, 0 tests failed out of 4

Total Test time (real) =  88.79 sec
```
