#include "MeshUtils.h"
#include "TSurfReader.h"

#include "../demo/BunnyMesh.h"

#include <QFileInfo>

#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLPolyDataReader.h>

vtkSmartPointer<vtkPolyData> withNormals(vtkPolyData *poly)
{
    vtkNew<vtkPolyDataNormals> normals;
    normals->SetInputData(poly);
    normals->SplittingOff();
    normals->ConsistencyOn();
    normals->Update();
    return normals->GetOutput();
}

void addCoordinateArrays(vtkPolyData *poly)
{
    const char *names[] = {"X", "Y", "Z"};
    const vtkIdType n = poly->GetNumberOfPoints();
    for (int comp = 0; comp < 3; ++comp) {
        if (poly->GetPointData()->GetArray(names[comp]))
            continue;
        auto array = vtkSmartPointer<vtkFloatArray>::New();
        array->SetName(names[comp]);
        array->SetNumberOfTuples(n);
        for (vtkIdType i = 0; i < n; ++i)
            array->SetValue(i, float(poly->GetPoint(i)[comp]));
        poly->GetPointData()->AddArray(array);
    }
}

vtkSmartPointer<vtkPolyData> bunnyPolyData()
{
    vtkNew<vtkPoints> points;
    for (std::size_t v = 0; v < bunny::vertexCount; ++v)
        points->InsertNextPoint(bunny::positions[3 * v],
                                bunny::positions[3 * v + 1],
                                bunny::positions[3 * v + 2]);
    vtkNew<vtkCellArray> tris;
    for (std::size_t f = 0; f + 2 < bunny::indexCount; f += 3) {
        tris->InsertNextCell(3);
        tris->InsertCellPoint(bunny::indices[f]);
        tris->InsertCellPoint(bunny::indices[f + 1]);
        tris->InsertCellPoint(bunny::indices[f + 2]);
    }
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    poly->SetPoints(points);
    poly->SetPolys(tris);
    return poly;
}

namespace {

template <typename Reader>
vtkSmartPointer<vtkPolyData> readWith(const QString &path)
{
    vtkNew<Reader> reader;
    reader->SetFileName(path.toUtf8().constData());
    reader->Update();
    return reader->GetOutput();
}

} // namespace

vtkSmartPointer<vtkPolyData> loadMeshFile(const QString &path, QString *error)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    vtkSmartPointer<vtkPolyData> poly;

    if (suffix == "ts")
        poly = tsurf::read(path, error);
    else if (suffix == "vtk")
        poly = readWith<vtkPolyDataReader>(path);
    else if (suffix == "vtp")
        poly = readWith<vtkXMLPolyDataReader>(path);
    else if (suffix == "ply")
        poly = readWith<vtkPLYReader>(path);
    else if (suffix == "obj")
        poly = readWith<vtkOBJReader>(path);
    else if (error)
        *error = QStringLiteral("unsupported format: .%1").arg(suffix);

    if (poly && poly->GetNumberOfPoints() == 0) {
        if (error && error->isEmpty())
            *error = QStringLiteral("%1: empty mesh").arg(path);
        poly = nullptr;
    }
    return poly;
}
