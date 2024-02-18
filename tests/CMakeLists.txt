enable_testing()

add_executable(tests tests.cpp)

add_test(NAME Test1 COMMAND tests)