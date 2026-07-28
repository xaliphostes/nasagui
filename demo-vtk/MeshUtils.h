#pragma once

#include <QString>

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

// Recompute smooth per-vertex normals (returns a new polydata).
vtkSmartPointer<vtkPolyData> withNormals(vtkPolyData *poly);

// Add "X"/"Y"/"Z" coordinate point arrays if absent, so every object has
// plottable attributes.
void addCoordinateArrays(vtkPolyData *poly);

// The embedded Stanford bunny (demo/BunnyMesh.h) as polydata.
vtkSmartPointer<vtkPolyData> bunnyPolyData();

// Load *.vtk, *.vtp, *.ply, *.obj or Gocad TSurf *.ts.
// Returns null and sets *error on failure.
vtkSmartPointer<vtkPolyData> loadMeshFile(const QString &path, QString *error);
