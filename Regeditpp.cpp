#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

namespace RegeditPP
{
    /**
     * @brief The key name includes the absolute path of the key in the registry, always starting at
     * a base key, for example, HKEY_LOCAL_MACHINE
     */
    constexpr auto inline KayNameMax = 255;

    constexpr auto inline ValueNameMax = 16383;

    constexpr auto inline DepthMax = 512;

    constexpr auto inline DepthMaxInSingleCall = 32;

    /*
        Additional Note:
        1. Each blackslash in the location string must preceded by another blackslash as an escape character.
         For example C:\\mydir\\myfile.txt
        2. Key names cannot contain blackslashes.
    */

    /**
     * @brief Defined in <winnt.h>
     */
    enum class Type
    {
        Binary = REG_BINARY,
        Dword = REG_DWORD,
        DwordBigEndian = REG_DWORD_BIG_ENDIAN,
        UnexpandedString = REG_EXPAND_SZ,
        Link = REG_LINK,
        MultiString = REG_MULTI_SZ,
        None = REG_NONE,
        Qword = REG_QWORD,
        String = REG_SZ
    };

    struct ValueBase
    {
        std::wstring_view m_name;
    };

    template<Type T>
    class Value;

    template<>
    class Value<Type::Binary> : public ValueBase
    {
    public:
        Value(std::wstring_view name, std::vector<BYTE>&& value)
            : ValueBase{name}
            , m_value{std::move(value)}
        {
        }
    private:
        std::vector<BYTE> m_value;
    };


    class Key
    {
        HKEY m_keyHandle{};
        bool m_keepAlive{};

        Key(std::wstring_view subKey, bool keepAlive) : m_keepAlive{ keepAlive }
        {
            auto const result = RegOpenKeyExW(
                m_keyHandle,
                subKey.data(),
                0,
                KEY_READ,       //TODO
                &m_keyHandle
            );
            if (result != ERROR_SUCCESS)
            {
                throw std::runtime_error("RegOpenKeyExW failed");
            }
        }
    public:
        constexpr Key(HKEY keyHandle) : m_keyHandle(keyHandle) {}
        
        Key operator[](std::wstring_view subKey) const
        {
            return Key{subKey, true};
        }

        template<Type ValueType>
        Key& operator+=(Value<ValueType> const& value)
        {
            return *this;
        }

        Key& operator-=(std::wstring_view valueName)
        {
            return *this;
        }
        
        ~Key()
        {
            if (!m_keepAlive)
                [[maybe_unused]]auto const result = RegCloseKey(m_keyHandle);
        }
    };

    constexpr Key LocalMachine{ HKEY_LOCAL_MACHINE };
    constexpr Key ClassesRoot{ HKEY_CLASSES_ROOT };
    constexpr Key CurrentUser{ HKEY_CURRENT_USER };
    constexpr Key Users{ HKEY_USERS };
}

void say_hello()
{
    //Opening keys
    auto const key = RegeditPP::LocalMachine[L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"];
    
    //Reading values
    auto const value = key[L"Test"];
    
    //Iterating through key
    for (auto const& subKey : key)
    {
        std::wcout << subKey.m_name << std::endl;
    }

    //Creating keys
    auto const newKey = key[L"NewKey"];
    
    //Writing Value
    newKey += RegeditPP::Value<RegeditPP::Type::String>{ L"Test", L"Hello" };
    
    //Deleting value
    newKey -= L"Test";
}
