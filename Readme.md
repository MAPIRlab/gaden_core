# Gaden core library
Gaden is a gas dispersion simulator! This repo contains the backend core library, which can also be included directly in your project, or accessed through the [ROS](https://github.com/MAPIRlab/gaden) and [GUI](https://github.com/MAPIRlab/gaden_gui) frontends.

To-do: the rest of the readme :)

## Python bindings
You can generate Python bindings for the gaden core library with [cppyy](https://cppyy.readthedocs.io/en/latest/). To generate them, simply enable the corresponding option in the [CMakeLists.txt](CMakeLists.txt).

Creating these bindings requires the following dependencies:

```
pip install wheel
pip install cppyy
pip install libclang==13
sudo apt install libclang-13-dev
```

### Usage example
See [this folder](examples/pythonAPI/Readme.md).