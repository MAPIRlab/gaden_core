# Gaden core library
Gaden is a gas dispersion simulator! This repo contains the backend core library, which can also be included directly in your C++ project, or accessed through the [ROS](https://github.com/MAPIRlab/gaden) and [GUI](https://github.com/MAPIRlab/gaden_gui) frontends.

See the [tutorial](examples/tutorial/) for basic info on the data types that this library defines and how to use them.

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

TO-DO detailed instructions for usage in C++


## Python bindings
You can generate Python bindings for the gaden core library with [cppyy](https://cppyy.readthedocs.io/en/latest/). To generate them, simply enable the corresponding option in the [CMakeLists.txt](CMakeLists.txt) before building.

Creating these bindings requires the following dependencies:

```
pip install wheel
pip install cppyy
pip install libclang==13
sudo apt install libclang-13-dev
```

### Usage example
See [this folder](examples/pythonAPI/Readme.md).