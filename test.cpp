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
    auto const value = key.valueOf(L"stringValue").as<Type::String>();
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
TEST(Read, UnexpandedStringValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"unexpandedStringValue").as<Type::UnexpandedString>();
    auto const originalString = value.getValue();
    auto const expandedString = value.expand();
    EXPECT_NE(originalString, expandedString);
}
TEST(Read, MultiStringValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"multiString").as<Type::MultiString>();
    EXPECT_EQ(value.getValue(), L"multi\0string\0");
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
    auto key = CurrentUser[L"test"][L"EnumKeys"];
    std::array const expectedName{
        L"subkey"
        L"binaryValue",
        L"dwordValue",
        L"qwordValue",
        L"stringValue",
        L"multiStringValue",
        L"UnexpandedStringValue"
    };
    std::array const expectedType
    {
        
        Type::Binary,
        Type::Dword,
        Type::Qword,
        Type::String,
        Type::MultiString,
        Type::UnexpandedString
    };
    
    std::set<std::wstring> actualName;
    std::set<Type> actualTypes;

    for (auto child : key)
    {
        std::visit(
            [&](auto&& node)
            {
                if constexpr (!std::is_same_v<std::decay_t<decltype(node)>, Key>)
                {
                    actualName.insert(std::wstring{ node.getName() });
                    actualTypes.insert(node.ValueType_v);
                }
            }, 
            child
        );
    }
    EXPECT_EQ(actualName.size(), expectedName.size());
    EXPECT_EQ(actualTypes.size(), expectedType.size());
}

TEST(Enum, EnumEmpty)
{
    auto key = CurrentUser[L"test"][L"empty"];
    EXPECT_EQ(key.begin(), key.end());
}

//Rename key
TEST(Rename, RenameKey)
{
    auto key = CurrentUser[L"test"][L"oldName"];
    key.rename(L"newName");
    EXPECT_NO_THROW(CurrentUser[L"test"][L"newName"]);
}

TEST(Additional, SplitString)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"].valueOf(L"multiString").as<Type::MultiString>();
    auto const splitResult = key.split();
    EXPECT_EQ(splitResult[0], "multi");
    EXPECT_EQ(splitResult[1], "string");
}



int main(int argc, char **argv) 
{
    std::cout << "Start testing\n";
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}