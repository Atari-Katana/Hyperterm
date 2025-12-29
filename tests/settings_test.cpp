#include <gtest/gtest.h>
#include "settings/settings.hpp"

TEST(SettingsTest, SetAndGetString) {
    Settings s;
    s.setString("test.key", "test_value");
    ASSERT_EQ(s.getString("test.key", "default"), "test_value");
}

TEST(SettingsTest, GetStringDefault) {
    Settings s;
    ASSERT_EQ(s.getString("nonexistent.key", "default_value"), "default_value");
}

TEST(SettingsTest, SetAndGetInt) {
    Settings s;
    s.setInt("test.int.key", 123);
    ASSERT_EQ(s.getInt("test.int.key", 0), 123);
}

TEST(SettingsTest, InvalidIntReturnsDefault) {
    Settings s;
    s.setString("test.invalid.int", "not-an-int");
    // We expect a warning to be printed to stderr, but the test should pass.
    ASSERT_EQ(s.getInt("test.invalid.int", 42), 42);
}
