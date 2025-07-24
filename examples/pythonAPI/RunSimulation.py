from gaden_py import gaden

# Read the configuration metadata from the yaml files
# These files can be edited by hand, or created with https://github.com/MAPIRlab/gaden_gui
configMetadata = gaden.EnvironmentConfigMetadata("../example_project/environment_configurations/config1")
configMetadata.ReadDirectory()

# Create the configuration object (environment + wind data)
envConfig = gaden.Preprocessing.Preprocess(configMetadata)

# retrieve the configuration parameters (read from the yaml file) by name. 
# Alternatively, you can create a new gaden.RunningSimulation.Parameters object and populate it manually
simParams = configMetadata.GetSimulationParams("sim1")

simulation = gaden.RunningSimulation(simParams, envConfig)

while simulation.GetCurrentTime() < 300:
    print("Progress: {:.2f}/300.0s".format(simulation.GetCurrentTime()), end="\r")
    simulation.AdvanceTimestep()
print("Simulation finished!             ")