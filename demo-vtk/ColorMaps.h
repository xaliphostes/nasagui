#pragma once

#include <QBrush>       // QGradientStops
#include <QStringList>

#include <vtkLookupTable.h>
#include <vtkSmartPointer.h>

// Color-table presets shared by the properties panel
// (same values as nasagui::ModelView::ColorMap).

QStringList colorMapNames();
QGradientStops colorMapStops(int index);
vtkSmartPointer<vtkLookupTable> makeLookupTable(int colorMapIndex);
