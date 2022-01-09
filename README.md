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
auto controlPanel = CurrentUser[L"ControlPanel"];
```
The `[]` operator can be chanined to open subkeys at any level in one line.
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
### Enumerating Registry Subkeys
To enumerate a key, you can either use the iterator return from `.begin()` and `.end()`,
or a range-based `for` loop, which uses the iterator underhood.
When the iterator is dereferenced with operator`*`, it returns a `std::variant` of all the
supported value-types as well as the key type. 
Then you may use `std::visit()` to call different functions
for different value types.
```cpp
for(auto child : key) //child is a std::variant
{
    std::visit()
}
```
### Deleting a Key with Subkeys
### Determining the Registry Size

## Values


## Inter-op
This small header-only library is written with inter-op with Win32 APIs in mind for greater flexibility.
There is a getter function called `.getXYZ()` on multiple wrapper classes. For example
```cpp
HKEY keyHandle = key.getHandle();

```