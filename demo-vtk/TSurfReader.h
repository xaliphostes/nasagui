#pragma once

#include <QString>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

// Minimal Gocad TSurf (.ts) reader: VRTX / PVRTX / ATOM vertices, TRGL
// triangles, PROPERTIES columns become point-data float arrays. Multiple
// TSurf objects in one file are merged.
namespace tsurf {

vtkSmartPointer<vtkPolyData> read(const QString &path, QString *error = nullptr);

} // namespace tsurf
