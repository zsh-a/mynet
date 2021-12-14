
// #define CATCH_CONFIG_MAIN
#include"mynet/add.h"
#include"gtest/gtest.h"
#include<iostream>
// #include "catch2/catch.hpp"


// TEST_CASE( "simple" )
// {
//     REQUIRE( add(1,2) == 3 );
// }

TEST(hello,world){

    EXPECT_EQ(add(2,2),4);
    // std::cout << "hello\n";
}