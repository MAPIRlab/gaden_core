
execute_process(COMMAND cling-config --cmake OUTPUT_VARIABLE CPPYY_MODULE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
message("CPYY_MODULE_PATH: " ${CPPYY_MODULE_PATH})
list(INSERT CMAKE_MODULE_PATH 0 ${CPPYY_MODULE_PATH})

find_package(Cppyy)
find_package(LibClang)

# Note this is a necessary compile flag for cppyy bindings to work.
set_target_properties(gaden PROPERTIES POSITION_INDEPENDENT_CODE ON)
get_target_property(GADEN_INCLUDES gaden INCLUDE_DIRECTORIES)

cppyy_add_bindings(
   "gaden_py" "3.0.0" "Pepe Ojeda" "ojedamorala@uma.es"
   LANGUAGE_STANDARD "20"
   GENERATE_OPTIONS "-D__PIC__;-Wno-macro-redefined"
   INCLUDE_DIRS ${GADEN_INCLUDES}
   LINK_LIBRARIES gaden
   H_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
   H_FILES  "include/gaden/AirflowDisturbance.hpp"
            "include/gaden/Environment.hpp"
            "include/gaden/EnvironmentConfigMetadata.hpp"
            "include/gaden/EnvironmentConfiguration.hpp"
            "include/gaden/gaden.hpp"
            "include/gaden/PlaybackSimulation.hpp"
            "include/gaden/Preprocessing.hpp"
            "include/gaden/RunningSimulation.hpp"
            "include/gaden/Simulation.hpp"
)