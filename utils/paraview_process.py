import os
import sys
from paraview.simple import *


def is_decomposed_openfoam_case(foam_case):
    case_dir = foam_case if os.path.isdir(foam_case) else os.path.dirname(os.path.abspath(foam_case))
    if not case_dir:
        return False
    return any(
        os.path.isdir(os.path.join(case_dir, d))
        for d in os.listdir(case_dir)
        if d.startswith("processor")
    )


def compute_sampling_dimensions(foam, resolution):
    foam.UpdatePipeline()
    info = foam.GetDataInformation()
    bounds = info.GetBounds()
    lengths = [bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]]

    dimensions = [int(round(length / resolution)) for length in lengths]
    return dimensions


def main():
    if len(sys.argv) not in (3, 4):
        print("Usage: python script.py <OpenFOAM_case.foam> <output.csv> [resolution = 0.1]")
        return

    foam_case = sys.argv[1]
    output_csv = sys.argv[2]
    resolution = 0.1 if len(sys.argv) < 4 else float(sys.argv[3])

    print(f"Reading file: {foam_case}...")
    if is_decomposed_openfoam_case(foam_case):
        CaseType = 'Decomposed Case'
    else:
        CaseType = 'Reconstructed Case'
        
    foam = OpenFOAMReader(FileName=[foam_case], CaseType=CaseType)
    foam.CellArrays = ['U']

    print(f"Resampling with resolution: {resolution}m... This could take a while.")
    resampled = ResampleToImage(Input=foam)
    resampled.SamplingDimensions = compute_sampling_dimensions(foam, resolution)

    # Convert to a point cloud object to include the position of the samples in the export file
    point_cloud = ConvertToPointCloud(Input=resampled)
    point_cloud.UpdatePipeline()

    # We need to manually create the writer, rather than call SaveData(), to be able to choose the U array as the only data to be exported 
    writer = CreateWriter(output_csv, point_cloud)
    writer.ChooseArraysToWrite = 1
    writer.FieldAssociation = 'Point Data'
    writer.PointDataArrays = ['U']
    writer.CellDataArrays = []
    writer.UpdatePipeline()
    print(f"Saved point cloud to {output_csv}")


if __name__ == "__main__":
    main()
