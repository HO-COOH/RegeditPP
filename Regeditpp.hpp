#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <string_view>
#include <variant>
#include <optional>
#include <cassert>

using QWORD = uint64_t;

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
        UnexpandedString = REG_EXPAND_SZ,
        MultiString = REG_MULTI_SZ,
        Qword = REG_QWORD,
        String = REG_SZ,

        //Additional types
        DwordBigEndian = REG_DWORD_BIG_ENDIAN,
        Link = REG_LINK,
        None = REG_NONE,
    };

    template<Type T>
    class Value;

    template<typename This>
    struct ValueBase
    {
    private:
        template<typename T>
        struct GetValueTypeFromThis;

        template<Type type>
        struct GetValueTypeFromThis<Value<type>>
        {
            constexpr static auto inline ValueType_v = type;
        };
    public:

        constexpr static auto inline ValueType_v = GetValueTypeFromThis<This>::ValueType_v;

        std::wstring m_name;

        ValueBase(std::wstring_view name) : m_name{ std::wstring{name} }
        {
        }

        ValueBase(std::wstring name) : m_name{ std::move(name) }
        {
        }

        auto const& get() const
        {
            return static_cast<This const*>(this)->m_value;
        }

        auto const& getName() const
        {
            return m_name;
        }
    };



    template<>
    class Value<Type::Binary> : public ValueBase<Value<Type::Binary>>
    {
        std::vector<BYTE> m_value;
        friend struct ValueBase<Value<Type::Binary>>;
    public:
        Value(std::wstring name, std::vector<BYTE>&& value): 
            ValueBase{std::move(name)},
            m_value{std::move(value)}
        {
        }

        auto getPointer() const
        {
            return m_value.data();
        }

        auto getSize() const
        {
            return m_value.size() * sizeof(m_value.front());
        }
    };


    template<>
    class Value<Type::Dword> : public ValueBase<Value<Type::Dword>>
    {
        DWORD m_value;
        friend struct ValueBase<Value<Type::Dword>>;
    public:
        Value(std::wstring name, DWORD value):
            ValueBase{ std::move(name) },
            m_value{value}
        {
        }

        constexpr auto getSize() const
        {
            return sizeof(DWORD);
        }
        
        constexpr auto getPointer() const
        {
            return &m_value;
        }
    };

    template<>
    class Value<Type::Qword> : public ValueBase<Value<Type::Qword>>
    {
        QWORD m_value;
        friend struct ValueBase<Value<Type::Qword>>;
    public:
        Value(std::wstring name, QWORD value):
            ValueBase{ std::move(name) },
            m_value{ value }
        {
        }

        constexpr auto getSize() const
        {
            return sizeof(QWORD);
        }

        constexpr auto getPointer() const
        {
            return &m_value;
        }
    };

    template<Type type>
    class StringValueBase : public ValueBase<Value<type>>
    {
    protected:
        std::wstring m_value;
        friend struct ValueBase<Value<type>>;
    public:
        StringValueBase(std::wstring name, std::wstring&& value):
            ValueBase<Value<type>>{std::move(name)},
            m_value{std::move(value)}
        {
        }

        auto getSize() const
        {
            return m_value.size() * sizeof(m_value.front());
        }

        auto getPointer() const
        {
            return &m_value[0];
        }
    };

    template<>
    class Value<Type::String> : public StringValueBase<Type::String>
    {
        friend struct ValueBase<Value<Type::String>>;
    public:
        Value(std::wstring name, std::wstring&& value)
            : StringValueBase{ std::move(name), std::move(value) }
        {
        }
    };

    template<>
    class Value<Type::MultiString> : public StringValueBase<Type::MultiString>
    {
        friend struct ValueBase<Value<Type::MultiString>>;
    public:
        Value(std::wstring name, std::wstring&& value)
            : StringValueBase{ std::move(name), std::move(value) }
        {
        }

        std::vector<std::wstring> split() const
        {
            auto iter = m_value.cbegin();
            std::vector<std::wstring> result;
            
            size_t start = 0;
            for (size_t i = 0; i < m_value.size(); ++i)
            {
                if (m_value[i] == L'\0' || i == m_value.size() - 1)
                {
                    result.push_back(m_value.substr(start, i - start));
                    start = i + 1;
                }
            }
            return result;
        }
    };

    template<>
    class Value<Type::UnexpandedString> : public StringValueBase<Type::UnexpandedString>
    {
        friend struct ValueBase<Value<Type::UnexpandedString>>;
    public:
        Value(std::wstring name, std::wstring&& value)
            : StringValueBase{ std::move(name), std::move(value)}
        {
        }

        std::wstring expand() const
        {
            //first get the required length for buffer
            auto const length = ExpandEnvironmentStringsW(
                getPointer(),
                nullptr,
                0
            );
            std::wstring expanded(length, 0);
            assert(ExpandEnvironmentStringsW(getPointer(), &expanded[0], length) == length);
            return expanded;
        }
    };


    struct AccessRight
    {
        constexpr auto static inline AllAccess = KEY_ALL_ACCESS;
        //constexpr auto static inline CreateLink = KEY_CREATE_LINK; //reserved
        constexpr auto static inline Subkey = KEY_CREATE_SUB_KEY;
        constexpr auto static inline EnumerateSubKeys = KEY_ENUMERATE_SUB_KEYS;
        constexpr auto static inline Execute = KEY_EXECUTE;
        constexpr auto static inline Notify = KEY_NOTIFY;
        constexpr auto static inline QueryValue = KEY_QUERY_VALUE;
        constexpr auto static inline Read = KEY_READ;
        constexpr auto static inline SetValue = KEY_SET_VALUE;
        constexpr auto static inline View64Bit = KEY_WOW64_64KEY;
        constexpr auto static inline View32Bit = KEY_WOW64_32KEY;
        constexpr auto static inline Write = KEY_WRITE;

        constexpr auto static inline Default = AllAccess;
    };

    class Key
    {
        HKEY m_keyHandle{};
        bool m_keepAlive = true;

        Key(HKEY const currentKey, std::wstring_view subKey, bool keepAlive) : m_keepAlive{ keepAlive }
        {
            if (!subKey.empty())
            {
                auto const result = RegOpenKeyExW(
                    currentKey,
                    subKey.data(),
                    0,
                    AccessRight::Default,       //TODO
                    &m_keyHandle
                );
                if (result != ERROR_SUCCESS)
                {
                    throw std::runtime_error("RegOpenKeyExW failed");
                }
            }
        }

        class UnspecifiedValue
        {
            std::wstring name;
            HKEY m_keyHandle;

            template<typename T>
            auto valueAs_impl(std::wstring_view name, T* ptr, DWORD bytes) const
            {
                RegQueryValueExW(
                    m_keyHandle,
                    name.data(),
                    0,
                    nullptr,
                    reinterpret_cast<BYTE*>(ptr),
                    &bytes
                );
            }
            std::optional<Type> m_type;
        public:
            UnspecifiedValue(std::wstring name, HKEY key) :
                name{std::move(name)},
                m_keyHandle{key}
            {}

            template<Type type>
            auto as() const
            {
                if constexpr (type == Type::Binary)
                {
                    DWORD typeValue{};
                    DWORD bytes{};

                    RegQueryValueExW(
                        m_keyHandle,    //hkey
                        name.data(),    //lpValueName
                        0,              //lpReserved
                        &typeValue,          //lpType
                        nullptr,        //lpData
                        &bytes          //lpcbData
                    );

                    std::vector<BYTE> data(bytes, 0);
                    valueAs_impl(name, &data[0], bytes);
                    return Value<Type::Binary>{std::move(name), std::move(data)};
                }
                else if constexpr (type == Type::Dword)
                {
                    DWORD data{};
                    DWORD bytes = sizeof(DWORD);
                    RegQueryValueExW(
                        m_keyHandle,
                        name.data(),
                        0,
                        nullptr,
                        reinterpret_cast<BYTE*>(&data),
                        &bytes
                    );
                    return Value<Type::Dword>{std::move(name), data};
                }
                else if constexpr (type == Type::Qword)
                {
                    QWORD data{};
                    DWORD bytes = sizeof(QWORD);
                    RegQueryValueExW(
                        m_keyHandle,
                        name.data(),
                        0,
                        nullptr,
                        reinterpret_cast<BYTE*>(&data),
                        &bytes
                    );
                    return Value<Type::Qword>{std::move(name), data};
                }
                else if constexpr (type == Type::String || type == Type::MultiString || type == Type::UnexpandedString)
                {
                    DWORD typeValue{};
                    DWORD bytes{};

                    RegQueryValueExW(
                        m_keyHandle,    //hkey
                        name.data(),    //lpValueName
                        0,              //lpReserved
                        &typeValue,          //lpType
                        nullptr,        //lpData
                        &bytes          //lpcbData
                    );

                    std::wstring value((bytes / sizeof(wchar_t)) - 1, 0);
                    valueAs_impl(name, &value[0], bytes);
                    return Value<type>{std::move(name), std::move(value)};
                }
            }

            template<Type type> 
            void operator=(Value<type> const& value)
            {
                RegSetValueExW(
                    m_keyHandle,                                    //hkey
                    name.data(),                                    //lpValueName
                    0,                                              //reserved
                    static_cast<DWORD>(type),                       //dwType
                    reinterpret_cast<BYTE const*>(value.getPointer()), //lpData
                    value.getSize()                                 //cbData
                );
            }

            Type getType()
            {
                if (m_type.has_value())
                    return *m_type;

                Type type{};
                RegQueryValueExW(
                    m_keyHandle,
                    name.data(),
                    0,
                    reinterpret_cast<DWORD*>(&type),
                    nullptr,
                    nullptr
                );
                m_type = type;
                return type;
            }

            template<Type type>
            bool is()
            {
                return type == getType();
            }
        };
    public:
        constexpr Key(HKEY keyHandle) : m_keyHandle(keyHandle) {}


        template<Type ValueType>
        Key& operator+=(Value<ValueType> const& value)
        {
            RegSetValueExW(
                m_keyHandle,                    //hkey
                value.m_name.data(),            //lpValueName
                0,                              //Reserved
                static_cast<DWORD>(ValueType),  //dwType
                reinterpret_cast<BYTE const*>(value.getPointer()),         //lpData
                value.getSize()
            );
            return *this;
        }


        Key& operator-=(std::wstring_view valueName)
        {
            RegDeleteValueW(
                m_keyHandle,        //hkey
                valueName.data()    //lpValueName
            );
            return *this;
        }


        void remove(std::wstring_view subKey, bool recursive = false) const
        {
            if (recursive)
            {
                RegDeleteTreeW(
                    m_keyHandle,    //hkey
                    subKey.data()
                );
            }
            else
            {
                RegDeleteKeyW(
                    m_keyHandle,    //hkey
                    subKey.data()   //lpSubKey
                );
            }
        }

        void remove() const
        {
            RegDeleteTreeW(
                m_keyHandle,
                nullptr
            );
        }

        ~Key()
        {
            if (!m_keepAlive)
                [[maybe_unused]] auto const result = RegCloseKey(m_keyHandle);
        }

        using ValueVariant = std::variant<
            Value<Type::Binary>,
            Value<Type::Dword>,
            //Value<Type::DwordBigEndian>,
            Value<Type::UnexpandedString>,
            //Value<Type::Link>,
            Value<Type::MultiString>,
            //Value<Type::None>,
            Value<Type::Qword>,
            Value<Type::String>,
            Key
        >;

        struct ChildCount
        {
            DWORD subKeys{};
            DWORD subKeyMaxLength{};
            DWORD values{};
            DWORD valueMaxLength{};
        };

        class Iterator
        {
            DWORD m_index{};
            HKEY m_keyHandle{};
            ChildCount m_count;
        public:
            Iterator(DWORD index, HKEY key, ChildCount count) :
                m_index{index},
                m_keyHandle{key},
                m_count{count}
            {
            }
            auto operator==(Iterator const& rhs) const
            {
                return m_index == rhs.m_index && m_keyHandle == rhs.m_keyHandle;
            }
            auto operator!=(Iterator const& rhs) const
            {
                return !(*this == rhs);
            }
            Iterator& operator++()
            {
                ++m_index;
                return *this;
            }
            Iterator& operator--()
            {
                --m_index;
                return *this;
            }
            ValueVariant operator*() const
            {
                //enumerate keys first
                if (m_index < m_count.subKeys)
                {
                    std::wstring keyName(m_count.subKeyMaxLength, 0);
                    DWORD bytes = m_count.subKeyMaxLength + 1;
                    RegEnumKeyExW(
                        m_keyHandle,
                        m_index,
                        &keyName[0],
                        &bytes,
                        0,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                    return ValueVariant{ std::in_place_type<Key>, Key{m_keyHandle, L"", true}};
                }
                else
                {
                    //enumerate values
                    std::wstring valueName(m_count.valueMaxLength, 0);
                    DWORD bytes = m_count.valueMaxLength + 1;
                    RegEnumValueW(
                        m_keyHandle,    //hkey
                        m_index - m_count.subKeys,        //dwIndex
                        &valueName[0],  //lpValueName
                        &bytes,         //lpcchValueName
                        0,              //lpReserved
                        nullptr,        //lpType
                        nullptr,        //lpData
                        nullptr         //lpcbData
                    );
                    //Get type
                    UnspecifiedValue value{ valueName, m_keyHandle };
                    switch (value.getType())
                    {
                        case Type::Binary:
                            return ValueVariant{ std::in_place_type<Value<Type::Binary>>, value.as<Type::Binary>() };
                        case Type::Dword:
                            return ValueVariant{ std::in_place_type<Value<Type::Dword>>, value.as<Type::Dword>() };
                        case Type::Qword:
                            return ValueVariant{ std::in_place_type<Value<Type::Qword>>, value.as<Type::Qword>() };
                        case Type::String:
                            return ValueVariant{ std::in_place_type<Value<Type::String>>, value.as<Type::String>() };
                        case Type::MultiString:
                            return ValueVariant{ std::in_place_type<Value<Type::MultiString>>, value.as<Type::MultiString>()};
                        case Type::UnexpandedString:
                            return ValueVariant{ std::in_place_type<Value<Type::UnexpandedString>>, value.as<Type::UnexpandedString>() };
                        default:
                            assert(false);  //Unknown type?
                    }
                }
            }
        };

        Key operator[](std::wstring_view subKey) const
        {
            return Key{ m_keyHandle, subKey, true };
        }
        Key operator[](wchar_t const* subKey) const
        {
            return this->operator[](std::wstring_view{ subKey });
        }
        ValueVariant operator[](DWORD index) const
        {
            return *Iterator{ index, m_keyHandle, getNumChild() };
        }

        operator bool() const
        {
            return m_keyHandle != NULL;
        }

        Key create(std::wstring_view subKey) const
        {
            HKEY keyHandle{};
            auto result = RegCreateKeyExW(
                m_keyHandle,
                subKey.data(),
                0,
                nullptr,
                0,
                AccessRight::Default,
                nullptr,
                &keyHandle,
                nullptr
            );
            return Key{keyHandle};
        }

        auto valueOf(std::wstring name) const
        {
            return UnspecifiedValue{ std::move(name), m_keyHandle };
        }


        void rename(std::wstring_view newName) const
        {
            RegRenameKey(
                m_keyHandle,
                nullptr,
                newName.data()
            );
        }

        void renameSubKey(std::wstring_view subKey, std::wstring_view newName) const
        {
            RegRenameKey(
                m_keyHandle,
                subKey.data(),
                newName.data()
            );
        }

        ChildCount getNumChild() const
        {
            ChildCount count{};
            RegQueryInfoKeyW(
                m_keyHandle,            //hkey
                nullptr,                //lpClass
                nullptr,                //lpcchClass
                0,                      //lpReserved
                &count.subKeys,         //lpcSubKeys
                &count.subKeyMaxLength, //lpcbMaxSubkeyLen
                nullptr,                //lpcbMaxClassLen
                &count.values,          //lpcValues
                &count.valueMaxLength,  //lpcbMaxValueLen
                nullptr,                //lpcbMaxValueLen
                nullptr,                //lpcbSecurityDescriptor
                nullptr                 //lpftLastWriteTime
            );
            return count;
        }

        auto begin() const
        {
            return Iterator{ 0, m_keyHandle, getNumChild() };
        }

        auto end() const
        {
            auto const numChild = getNumChild();
            return Iterator{ numChild.subKeys + numChild.values, m_keyHandle, {} };
        }

        [[nodiscard]]bool isReflectionEnabled() const
        {
            BOOL value{};
            RegQueryReflectionKey(
                m_keyHandle,    //hBase
                &value          //bIsReflectionDisabled
            );
            return !static_cast<bool>(value);
        }


        void flush() const
        {
            RegFlushKey(m_keyHandle);
        }

        [[nodiscard]] auto getHandle() const
        {
            return m_keyHandle;
        }
    };

    Key LocalMachine{ HKEY_LOCAL_MACHINE };
    Key ClassesRoot{ HKEY_CLASSES_ROOT };
    Key CurrentUser{ HKEY_CURRENT_USER };
    Key Users{ HKEY_USERS };
}
