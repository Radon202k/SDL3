# SDL3
You'll have to include SDL3 source inside app/jni, otherwise the build will fail.

Download the entire SDL3 source code at:
https://github.com/libsdl-org/SDL/releases

This example uses version 3.2.10

You'll also have to compile the shaders and put the .spv files in app/src/main/assets

Create the assets folder and put the .spv files in there.

To create the .spv files use, you can find the shader sources at app/jni/src/shaders:
```bash
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
```

To compile, after you have all the environment variables set (android sdk, java jdk, etc), your device connected and USB debug enabled, open up a command prompt, navigate to project root and run:
```bash
gradlew installDebug
```

Remember to use a USB 3.0 port.