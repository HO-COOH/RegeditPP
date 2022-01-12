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
    CompareBinary(value.get(), {0x01, 0x02, 0x03, 0x04});
}
TEST(Read, StringValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"stringValue").as<Type::String>();
    EXPECT_EQ(value.get(), L"string value");
}
TEST(Read, DwordValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"dwordValue").as<Type::Dword>();
    EXPECT_EQ(value.get(), 0x4d2);
}
TEST(Read, QwordValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"qwordValue").as<Type::Qword>();
    EXPECT_EQ(value.get(), 0x162e);
}
TEST(Read, UnexpandedStringValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"unexpandedStringValue").as<Type::UnexpandedString>();
    auto&& originalString = value.get();
    auto&& expandedString = value.expand();
    EXPECT_NE(originalString, expandedString);  //Can't really test this
}
TEST(Read, MultiStringValue)
{
    auto key = CurrentUser[L"test"][L"NewTestKey"];
    auto const value = key.valueOf(L"multiStringValue").as<Type::MultiString>();
    wchar_t const expectedResult[] = L"multi\0string\0";
    auto const actualResult = value.get();
    for (auto i = 0; i < actualResult.size(); ++i)
    {
        EXPECT_EQ(expectedResult[i], actualResult[i]);
    }
}


//Add a value
//TEST(Add, AddBinaryValue)
//{
//    auto testKey = CurrentUser[L"test"][L"AddValue"];
//    testKey += Value<Type::Binary>{L"binaryValue", { 0b1010, 0b0101 }};
//    testKey.flush();
//    CompareBinary(
//        testKey.valueOf(L"binaryValue").as<Type::Binary>().get(),
//        { 0b1010, 0b0101 }
//    );
//}
//TEST(Add, AddStringValue)
//{
//    auto key = CurrentUser[L"test"][L"AddValue"];
//    key += Value<Type::String>{L"stringValue", L"testValue"};
//    key.flush();
//    EXPECT_EQ(key.valueOf(L"stringValue").as<Type::String>().get(), L"testValue");
//}
//TEST(Add, AddDwordValue)
//{
//    auto key = CurrentUser[L"test"][L"AddValue"];
//    key += Value<Type::Dword>{L"dwordValue", 0x1234};
//    key.flush();
//    EXPECT_EQ(key.valueOf(L"dwordValue").as<Type::Dword>().get(), 0x1234);
//}
//TEST(Add, AddQwordValue)
//{
//    auto key = CurrentUser[L"test"][L"AddValue"];
//    key += Value<Type::Qword>{L"dwordValue", 0x1234};
//    key.flush();
//    EXPECT_EQ(key.valueOf(L"dwordValue").as<Type::Qword>().get(), 0x1234);
//}



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
    auto key = CurrentUser[L"test"][L"DeleteValue"];

    CompareBinary(key.valueOf(L"binaryValue").as<Type::Binary>().get(), { 0x01, 0x02, 0x03, 0x04 });

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
    EXPECT_EQ(splitResult[0], L"multi");
    EXPECT_EQ(splitResult[1], L"string");
}

static void CreateTestingEnvironment()
{
    auto testKey = CurrentUser.create(L"test");
    testKey.create(L"oldName");

    auto deleteValueSubkey = testKey.create(L"DeleteValue");
    deleteValueSubkey += Value<Type::Binary>(L"binaryValue", { 0x01, 0x02, 0x03, 0x04 });

    auto newTestSubKey = testKey.create(L"NewTestKey");
    newTestSubKey += Value<Type::Binary>(L"binaryValue", { 0x01, 0x02, 0x03, 0x04 });
    newTestSubKey += Value<Type::Dword>(L"dwordValue", 0x4d2);  //1234 decimal
    newTestSubKey += Value<Type::Qword>(L"qwordValue", 0x162e); //5678 decimal
    newTestSubKey += Value<Type::String>(L"stringValue", std::wstring{ L"string value" });
    newTestSubKey += Value<Type::UnexpandedString>(L"unexpandedStringValue", std::wstring{ L"%PATH%" });
    {
        wchar_t const value[] = L"multi\0string\0";
        newTestSubKey += Value<Type::MultiString>(L"multiStringValue", std::wstring{ std::begin(value), std::end(value) });
    }

}

int main(int argc, char **argv) 
{
    std::cout << "Start testing\n";
    CreateTestingEnvironment();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}