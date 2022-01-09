#include <gtest/gtest.h>
#include <array>
#include "Regeditpp.hpp"
#include <initializer_list>

using namespace RegeditPP;

//Reading a key
TEST(Read, ReadExistKey)
{
    auto key = CurrentUser[L"test"];
    EXPECT_TRUE(key);
}
TEST(Read, ReadNonExistKey)
{
    EXPECT_ANY_THROW(CurrentUser[L"test1"]);
}


//Creating a key
TEST(Create, CreateNewKey)
{
    auto newKey = CurrentUser[L"test"].create(L"NewTestKey1");
    EXPECT_TRUE(CurrentUser[L"test"][L"NewTestKey1"]);
}



//Read a value
void CompareBinary(std::vector<BYTE> const& vec, std::initializer_list<BYTE> expected)
{
    auto iter = expected.begin();
    for (auto byte : vec)
    {
        EXPECT_EQ(byte, *iter);
        ++iter;
    }
}
TEST(Read, BinaryValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"binaryValue").as<Type::Binary>();
    CompareBinary(value.getValue(), {0x01, 0x02, 0x03, 0x04});
}
TEST(Read, StringValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"stringValue\\0").as<Type::String>();
    EXPECT_EQ(value.getValue(), L"string value");
}
TEST(Read, DwordValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"dwordValue").as<Type::Dword>();
    EXPECT_EQ(value.getValue(), 0x4d2);
}
TEST(Read, QwordValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"qwordValue").as<Type::Qword>();
    EXPECT_EQ(value.getValue(), 0x162e);
}


//Add a value
TEST(Add, AddBinaryValue)
{
    auto testKey = CurrentUser[L"test"][L"AddValue"];
    testKey += Value<Type::Binary>{L"binaryValue", { 0b1010, 0b0101 }};
    testKey.flush();
    CompareBinary(
        testKey.valueOf(L"binaryValue").as<Type::Binary>().getValue(),
        { 0b1010, 0b0101 }
    );
}
TEST(Add, AddStringValue)
{
    auto key = CurrentUser[L"test"][L"AddValue"];
    key += Value<Type::String>{L"stringValue", L"testValue"};
    key.flush();
    EXPECT_EQ(key.valueOf(L"stringValue").as<Type::String>().getValue(), L"testValue");
}
TEST(Add, AddDwordValue)
{
    auto key = CurrentUser[L"test"][L"AddValue"];
    key += Value<Type::Dword>{L"dwordValue", 0x1234};
    key.flush();
    EXPECT_EQ(key.valueOf(L"dwordValue").as<Type::Dword>().getValue(), 0x1234);
}
TEST(Add, AddQwordValue)
{
    auto key = CurrentUser[L"test"][L"AddValue"];
    key += Value<Type::Qword>{L"dwordValue", 0x1234};
    key.flush();
    EXPECT_EQ(key.valueOf(L"dwordValue").as<Type::Qword>().getValue(), 0x1234);
}



//Delete a value
auto FindValueImpl(HKEY key, std::wstring_view name)
{
    return RegQueryValueExW(
        key,
        name.data(),
        0,
        nullptr,
        nullptr,
        nullptr
    );
}
TEST(DeleteValue, DeleteBinaryValue)
{
    auto key = CurrentUser[L"test"][L"AddValue"];
    key -= L"binaryValue";
    EXPECT_EQ(
        FindValueImpl(key.getHandle(), L"binaryValue"),
        ERROR_FILE_NOT_FOUND
    );
}


//Enum key
TEST(Enum, EnumKeys)
{

}


int main(int argc, char **argv) 
{
    std::cout << "Start testing\n";
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}