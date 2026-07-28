#pragma once

#include <nasagui/Theme.h>

#include <QColor>
#include <QString>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSmartPointer.h>

// One loaded object: mesh + rendering pipeline + display state.
struct SceneObject {
    QString name;
    vtkSmartPointer<vtkPolyData> data;
    vtkSmartPointer<vtkPolyDataMapper> mapper;
    vtkSmartPointer<vtkActor> actor;
    QColor color = nasagui::Theme::Primary;   // style default until the user picks one
    bool visible = true;
    int representation = 0;   // 0 surface, 1 surface+edges, 2 wireframe, 3 points
    QString activeArray;      // empty = solid color
    int colorMap = 0;
    double opacity = 1.0;
};
