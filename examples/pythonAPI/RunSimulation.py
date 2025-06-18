from gaden_py import gaden

# Read the configuration metadata from the yaml files
# These files can be edited by hand, or created with https://github.com/MAPIRlab/gaden_gui
configMetadata = gaden.EnvironmentConfigMetadata("../example_project/environment_configurations/config1")
configMetadata.ReadDirectory()

# Create the configuration object (environment + wind data). Function returns an std::optional, so use .value() to access the config object
envConfig = gaden.Preprocessing.Preprocess(configMetadata).value()

# retrieve the configuration parameters (they were read from the yaml file) by name. 
# Alternatively, you can create a new gaden.RunningSimulation.Parameters object and populate it manually
simParams = configMetadata.simulations.at("sim1")

simulation = gaden.RunningSimulation(simParams, envConfig)

while simulation.GetCurrentTime() < 300:
    simulation.AdvanceTimestep()

print("simulation finished!")