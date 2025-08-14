# lurk

**lurk** is a lightweight lua introspection / injection tool. It allows you to inspect, and dynamically interact with live Lua states from the outside—without patching the target application on disk.

## Usage
`lurk` is a shared library that is preloaded into a target process.
It waits for the lua library to be loaded and then dynamically replaces lua's state management functions.

### Parameters

`lurk` has the following parameters that should be passed by calling the exported `set_parameter()` right after loading.

On windows, this can be done by using [This](https://github.com/l41urie/tyrecon/tree/main/win_agent/installer) like so.
```
monitor.exe \
  -s "<Executable.exe>" \
  -a "lurk.dll" \
  -p set_parameters "dump {luasrc/}"
```

| Parameter      | Description |
|----------------|-------------|
| `dump`         | Dumps all Lua scripts loaded into any Lua state to a specified directory. |
| `load`         | Loads a specified Lua script into all active Lua states. |
| `periodic_func`| Specifies a Lua function (from the loaded script) to be periodically invoked across all Lua states. |
