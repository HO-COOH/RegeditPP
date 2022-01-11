# RegeditPP
A modern C++ wrapper for Win32 Registry utilities.

## Keys
### Pre-opened keys
[Microsoft docs](https://docs.microsoft.com/en-us/windows/win32/sysinfo/opening-creating-and-closing-keys).

- LocalMachine
- ClassesRoot
- Users
- CurrentUser

### Opening, Creating, and Closing Keys
You start from a pre-opened key (listed above) to open a subkey or create a subkey.
Opening a subkey is as simple as using the familiar array indexing operator`[L"subkey"]`.
For example, to open `HKEY_CURRENT_USER\Control Panel`, you do
```cpp
auto controlPanel = CurrentUser[L"Control Panel"];
```
The `[]` operator can be chanined to open subkeys at any level 
(up to 512, the maximum supported depth in registry) in a single line.
```cpp
auto currentVersion = CurrentUser[L"Software"][L"Microsoft"][L"Windows"][L"CurrentVersion"];
```
To create a subkey, you call `.create(L"subkey")` on an opened key.
```cpp
auto newKey = key.create(L"newKey");
```
You don't need to worry about closing the keys. 
It is handled automatically in the destructor.

If you need to explicitly write registry data to the hard disk, you call `.flush()`
```cpp
key.flush();
```

### Writing and Deleting Registry Data
Use the operator `+=Value<ValueType>(...)` to add a value and 
operator `-= L"valueName"` to delete a value. For example,
```cpp
auto run = CurrentUser[L"Software"][L"Microsoft"][L"Windows"][L"CurrentVersion"][L"Run"];
run -= L"LGHUB";    //Stop logitech g-hub auto launching when Windows start up

```


### Retriving Data from the Registry
There are 2 ways to read a value.
- If you are sure about the type, use `key.valueOf(L"valueName").as<type>()`
  The `.as<>()` call returns an instance of `Value<>` [type (see below)](###Types) 
  ```cpp
  auto binaryValue = 
      CurrentUser[L"Microsoft"][L"Windows"][L"CurrentVersion"][L"Explorer"][L"StartupApproved"][L"Run"]
      .valueOf(L"OneDriveSetup")
      .as<Type::Binary>();
  ```
- If you are **NOT** sure about the type, first get the type, then use a `switch` statement to handle it according to your need
  ```cpp
  auto unknownTypeValue = CurrentUser[L"..."];
  switch(unknownTypeValue.getType())
  {
      case Type::Binary:
      {
          auto binaryValue = unknownTypeValue.as<Type::Binary>();
          ...
      }
      ...
  }
  ```
### Enumerating Registry Subkeys
To enumerate a key, you can either use the iterator return from `.begin()` and `.end()`,
or a range-based `for` loop, which uses the iterator underhood.
When the iterator is dereferenced with operator`*`, it returns a `std::variant` of all the
supported value-types [(see below)](###Types) as well as the key type. 
Then you may use `std::visit()` to call different functions
for different value types.
```cpp
for(auto child : key) //child is a std::variant
{
    std::visit()
}
```
### Deleting a Key with Subkeys
Since `delete` is a keyword in C++, I chooose to use the word `.remove()` instead.
If you can be sure that there are no subkeys and values in the current key, you use
`.remove(L"key")`. Why not use operator`-=` you ask? 
Because there are no ways to distinguish whether to delete a key (fail if there are subkeys)
or to delete a key recursively using operator`-=` with a name. 
And in fact, operator `-=` is for [deleting a value from a key](Deleting a value from a key).

### Determining the Registry Size

### Values Types
See [docs](https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-value-types).
Values types are strongly-typed classes, with `Type` as its non-type template parameters.
You usually get an instance of this class from `key.valueOf(L"valueName").as<type>()`
All value types are listed as below
- `Value<Type::Binary>` -> `REG_BINARY`
- `Value<Type::Dword>` -> `REG_DWORD`
- `Value<Type::UnexpandedString>` -> `REG_EXPAND_SZ`
- `Value<Type::MultiString>` -> `REG_MULTI_SZ`
- `Value<Type::Qword>` -> `REG_QWORD`
- `Value<Type::String>` -> `REG_SZ`

All `Value<>` types support these methods:
- `getValue()` that returns the data that best expressed in C++
  + Binary type returns `std::vector<BYTE>`
  + Dword type returns `DWORD`
  + Qword type returns `QWORD`
  + String types returns `std::wstring`
- `getData()` returns a pointer to the underlying storage, which you should not modify
- `getName()` returns the name of the value

There are also additional methods that are supported in specific types, for example,
`Value<Type::UnexpanedString>` has `.expand()` that returns a string with the environment variables expanded. 
You can find out those methods with the assistance of your IDE.

## Inter-op
This small header-only library is written with inter-op with Win32 APIs in mind for greater flexibility.
There is a getter function called `.getXYZ()` on multiple wrapper classes. For example
```cpp
HKEY keyHandle = key.getHandle();

```