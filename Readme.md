# Gaden core library
Gaden is a gas dispersion simulator! This repo contains the backend core library, which can just be used directly in your C++ or Python project, or accessed through the [ROS](https://github.com/MAPIRlab/gaden) and [GUI](https://github.com/MAPIRlab/gaden_gui) frontends.

See the [tutorial](examples/tutorial/) for basic info on the data types that this library defines and how to use them.

## Video showcase
[![Video showcase](https://img.youtube.com/vi/2i3_pyV-MYU/hqdefault.jpg)](https://youtu.be/2i3_pyV-MYU)

## Installation and building
If you plan to use one of the frontends, don't download this repo separately. Instead, follow the instructions in the corresponding repo.

If you are going to use the library directly, clone the repo with
```
git clone --recursive git@github.com:MAPIRlab/gaden_core.git
```

You can then build the project as follows:
```
mkdir build
cd build 
cmake ..
make 
``` 

### Usage in a CMake project
You can use gaden in a C++ project with CMake as follows:

```
add_subdirectory([path]/gaden_core)
add_executable([your_exec])
target_link_libraries([your_exec] gaden)
```


## Python bindings
You can generate Python bindings for the gaden core library with [cppyy](https://cppyy.readthedocs.io/en/latest/). To generate them, simply enable the corresponding option in the [CMakeLists.txt](CMakeLists.txt) before building.

Creating these bindings requires setting up some dependencies first:

```
pip install wheel
pip install cppyy
pip install libclang
sudo apt install libclang-dev
```

When installing cppyy you might be prompted by a warning to add a specific path (like `/home/[user]/.local/bin`) to your `PATH`. You should listen to it!

If CMake complains about not being able to find libclang after the previous steps, you might need to manually create the `libclang.so` symlink:

```
# These steps are only required if CMake fails to find libclang.so
sudo apt install llvm
sudo ln -s $(llvm-config --prefix)/lib/libclang.so.1 $(llvm-config --prefix)/lib/libclang.so
```

### Usage example
See [this folder](examples/pythonAPI/Readme.md).
