# Example project

This directory outlines the basic structure of a gaden project. 

The file structure inside of `environment_configurations` (`config.yaml` and `simulations` folder) is enforced (and created automatically by gaden), but the rest (`cad_models`, `wind_simulations`, and even the name of the `environment_configurations` folder) can be modified. If any modifications are made, one should make sure to update the corresponding metadata in the `yaml` files.

The file `gaden.gproj` at the root of the project is not required for the use of the library, but is used by the [gui frontend](https://github.com/MAPIRlab/gaden_gui).