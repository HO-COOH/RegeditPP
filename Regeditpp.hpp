#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <string_view>
#include <variant>

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

    template<typename This>
    struct ValueBase
    {
        std::wstring_view m_name;

        auto const& getValue() const
        {
            return static_cast<This const*>(this)->m_value;
        }

    };

    template<Type T>
    class Value;

    template<>
    class Value<Type::Binary> : public ValueBase<Value<Type::Binary>>
    {
        std::vector<BYTE> m_value;
        friend struct ValueBase<Value<Type::Binary>>;
    public:
        Value(std::wstring_view name, std::vector<BYTE>&& value): 
            ValueBase{name},
            m_value{std::move(value)}
        {
        }

        auto getData() const
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
        Value(std::wstring_view name, DWORD value):
            ValueBase{name},
            m_value{value}
        {
        }

        constexpr auto getSize() const
        {
            return sizeof(DWORD);
        }
        
        constexpr auto getData() const
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
        Value(std::wstring_view name, QWORD value):
            ValueBase{ name },
            m_value{ value }
        {
        }

        constexpr auto getSize() const
        {
            return sizeof(QWORD);
        }

        constexpr auto getData() const
        {
            return &m_value;
        }
    };

    template<Type type>
    class StringValueBase : public ValueBase<Value<type>>
    {
        std::wstring m_value;
        friend struct ValueBase<Value<type>>;
    public:
        StringValueBase(std::wstring_view name, std::wstring&& value):
            ValueBase{name},
            m_value{std::move(value)}
        {
        }

        auto getSize() const
        {
            return m_value.size() * sizeof(m_value.front());
        }

        auto getData() const
        {
            return &m_value[0];
        }
    };

    template<>
    class Value<Type::String> : public StringValueBase<Type::String>
    {
        friend struct ValueBase<Value<Type::String>>;
    public:
        Value(std::wstring_view name, std::wstring&& value)
            : StringValueBase{ name, std::move(value) }
        {
        }
    };

    template<>
    class Value<Type::MultiString> : public StringValueBase<Type::MultiString>
    {
        friend struct ValueBase<Value<Type::MultiString>>;
    public:
        Value(std::wstring_view name, std::wstring&& value)
            : StringValueBase{ name, std::move(value) }
        {
        }
    };

    template<>
    class Value<Type::UnexpandedString> : public StringValueBase<Type::UnexpandedString>
    {
        friend struct ValueBase<Value<Type::UnexpandedString>>;
    public:
        Value(std::wstring_view name, std::wstring&& value)
            : StringValueBase{ name, std::move(value) }
        {
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

        class UnspecifiedValue
        {
            std::wstring_view name;
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

        public:
            constexpr UnspecifiedValue(std::wstring_view name, HKEY key) :
                name{name},
                m_keyHandle{key}
            {}

            template<Type type>
            auto as() const
            {
                if constexpr (type == Type::Binary)
                {
                    DWORD type{};
                    DWORD bytes{};

                    RegQueryValueExW(
                        m_keyHandle,    //hkey
                        name.data(),    //lpValueName
                        0,              //lpReserved
                        &type,          //lpType
                        nullptr,        //lpData
                        &bytes          //lpcbData
                    );

                    std::vector<BYTE> data(bytes, 0);
                    valueAs_impl(name, &data[0], bytes);
                    return Value<Type::Binary>{name, std::move(data)};
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
                    return Value<Type::Dword>{name, data};
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
                    return Value<Type::Qword>{name, data};
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

                    std::wstring value(bytes / sizeof(wchar_t), 0);
                    valueAs_impl(name, &value[0], bytes);
                    return Value<type>{name, std::move(value)};
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
                    reinterpret_cast<BYTE const*>(value.getData()), //lpData
                    value.getSize()                                 //cbData
                );
            }

            Type getType() const
            {
                Type type{};
                RegQueryValueExW(
                    m_keyHandle,
                    name.data(),
                    0,
                    reinterpret_cast<DWORD*>(&type),
                    nullptr,
                    nullptr
                );
                return type;
            }

            template<Type type>
            bool is() const
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
                reinterpret_cast<BYTE const*>(value.getData()),         //lpData
                value.getSize()
            );
            return *this;
        }

        Key& operator-=(std::wstring_view valueName)
        {
            return *this;
        }

        ~Key()
        {
            if (!m_keepAlive)
                [[maybe_unused]] auto const result = RegCloseKey(m_keyHandle);
        }

        using ValueVariant = std::variant<
            Value<Type::Binary>,
            Value<Type::Dword>,
            Value<Type::DwordBigEndian>,
            Value<Type::UnexpandedString>,
            Value<Type::Link>,
            Value<Type::MultiString>,
            Value<Type::None>,
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
                    DWORD bytes{};
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
                    return ValueVariant{ std::in_place_type<Key>, Key{m_keyHandle, keyName, true} };
                }
                else
                {
                    //enumerate values
                    std::wstring valueName(m_count.valueMaxLength, 0);
                    DWORD bytes{};
                    RegEnumKeyExW(
                        m_keyHandle,
                        m_index,
                        &valueName[0],
                        &bytes,
                        0,
                        nullptr,
                        nullptr,
                        nullptr
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

        void remove(std::wstring_view subKey) const
        {
            RegDeleteKeyW(
                m_keyHandle,    //hkey
                subKey.data()   //lpSubKey
            );
        }

        constexpr auto valueOf(std::wstring_view name) const
        {
            return UnspecifiedValue{ name, m_keyHandle };
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
            return Iterator{ numChild.subKeys + numChild.values, {}, {} };
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
