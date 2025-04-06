# SDL3
You'll have to include SDL3 source inside app/jni, otherwise the build will fail.

Download the entire SDL3 source code at:
https://github.com/libsdl-org/SDL/releases

This example uses version 3.2.10

You'll also have to compile the shaders and put the .spv files in app/src/main/assets

Create the assets folder and put the .spv files in there.

You can find the shader sources at app/jni/src/shaders.

To compile them into .spv files, use:
```bash
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
```

To test the app in a device, first make sure you have all the dependencies installed and the environment variables set (android sdk, java jdk, etc).

Also make sure your device is connected, and have USB debug enabled (use at least a USB 3.0 port).

Open up a command prompt, navigate to project root and run:
```bash
gradlew installDebug
```