### FFGLLightBrush

FFGLLightBrush is a video effect that enables light painting - bright spots will
stay on the screen. The plugin has been tested in Resolume Avenue, but should
work in other FFGL hosts as well. The effect offers three parameters, two
sliders and a button:

* **threshold** slider adjusts the threshold luminance - higher values will
  "burn" on the screen
* **darkening** slider adjusts how much the shadows will be darkened
* **clear** button clears the currently "burned" contents

### Building and Installing

A project file is included for Visual Studio Express 2013, which is a free
development environment for Windows. The plugin requires a recent version of
GLEW library (http://glew.sourceforge.net/). Consequently the installation is
not necessarily straightforward.

1. The GLEW include directory should be added to the compiler options (*Property
Pages > Configuration Properties > C/C++ > General > Additional Include
Directories*). Currently the project is configured to look from
*C:\Development\glew-1.12.0\include*.
2. After compilation, *FFGLLightBrush.dll* (from Debug directory) should be
placed in the plugin directory of the host application. In Resolume Avenua you
can configure plugin directories from *Avenue > Preferences > Video*.
3. *glew32.dll* (from *bin\Release\Win32*) may need to be copied in the same
directory with the host application. If there already is a *glew32.dll*, but the
plugin doesn't work, chances are that the you need to replace it with the newer
version. Rename the original DLL e.g. *glew32.dll.original*, and copy the new
DLL there.

### Author

Seppo Enarvi  
http://users.marjaniemi.com/seppo/
