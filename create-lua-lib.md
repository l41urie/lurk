# Creating the .lib for the present lua lib

Open up a "vs developer shell"

`dumpbin /exports "<APPLICATION FOLDER>\lua51.dll" /OUT:lua51.def` to create a .def

`lib /def:gamefolder/luaXX.def /OUT:lib/lua.lib /MACHINE:X64` to build the lib