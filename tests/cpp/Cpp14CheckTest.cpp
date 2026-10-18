// Cpp14CheckTest.cpp. Runs the C++14 translation unit.
#include <gtest/gtest.h>

bool Cpp14RoundTrip();

TEST(Protaco, Cpp14TranslationUnitWorks)
{
    EXPECT_TRUE(Cpp14RoundTrip());
}
