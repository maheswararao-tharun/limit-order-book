add_executable(lob_tests test_book.cpp test_itch_parser.cpp)
target_link_libraries(lob_tests PRIVATE lob_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(lob_tests)