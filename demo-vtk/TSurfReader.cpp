#include "TSurfReader.h"

#include <QFile>
#include <QHash>
#include <QTextStream>

#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>

namespace tsurf {

vtkSmartPointer<vtkPolyData> read(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("cannot open %1").arg(path);
        return nullptr;
    }

    auto points = vtkSmartPointer<vtkPoints>::New();
    auto triangles = vtkSmartPointer<vtkCellArray>::New();
    QHash<qlonglong, vtkIdType> idMap;          // TSurf vertex id -> point id
    QStringList propNames;                      // active PROPERTIES columns
    QHash<QString, QVector<float>> propValues;  // per-point values, zero-padded

    auto padProps = [&](vtkIdType pointCount) {
        for (auto it = propValues.begin(); it != propValues.end(); ++it)
            while (it.value().size() < pointCount)
                it.value().append(0.0f);
    };

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().simplified();
        if (line.isEmpty())
            continue;
        const QStringList tok = line.split(' ');
        const QString &kw = tok.first();

        if (kw == QLatin1String("PROPERTIES")) {
            propNames = tok.mid(1);
            for (const QString &name : propNames)
                if (!propValues.contains(name))
                    propValues.insert(name, QVector<float>());
        } else if ((kw == QLatin1String("VRTX") || kw == QLatin1String("PVRTX"))
                   && tok.size() >= 5) {
            const vtkIdType id = points->InsertNextPoint(
                tok[2].toDouble(), tok[3].toDouble(), tok[4].toDouble());
            idMap.insert(tok[1].toLongLong(), id);
            padProps(id);   // zero-fill any point added before these columns
            for (int i = 0; i < propNames.size(); ++i) {
                const int col = 5 + i;
                propValues[propNames[i]].append(
                    col < tok.size() ? tok[col].toFloat() : 0.0f);
            }
            padProps(id + 1);
        } else if (kw == QLatin1String("ATOM") && tok.size() >= 3) {
            const vtkIdType ref = idMap.value(tok[2].toLongLong(), -1);
            if (ref < 0)
                continue;
            double p[3];
            points->GetPoint(ref, p);
            const vtkIdType id = points->InsertNextPoint(p);
            idMap.insert(tok[1].toLongLong(), id);
            padProps(id);
            for (const QString &name : propValues.keys())
                propValues[name].append(propValues[name].value(int(ref), 0.0f));
            padProps(id + 1);
        } else if (kw == QLatin1String("TRGL") && tok.size() >= 4) {
            const vtkIdType a = idMap.value(tok[1].toLongLong(), -1);
            const vtkIdType b = idMap.value(tok[2].toLongLong(), -1);
            const vtkIdType c = idMap.value(tok[3].toLongLong(), -1);
            if (a >= 0 && b >= 0 && c >= 0) {
                triangles->InsertNextCell(3);
                triangles->InsertCellPoint(a);
                triangles->InsertCellPoint(b);
                triangles->InsertCellPoint(c);
            }
        }
        // HEADER {...}, TFACE, GOCAD, END, coordinate-system blocks: ignored
    }

    if (points->GetNumberOfPoints() == 0 || triangles->GetNumberOfCells() == 0) {
        if (error)
            *error = QStringLiteral("%1: no TSurf triangles found").arg(path);
        return nullptr;
    }

    auto poly = vtkSmartPointer<vtkPolyData>::New();
    poly->SetPoints(points);
    poly->SetPolys(triangles);

    padProps(points->GetNumberOfPoints());
    for (auto it = propValues.cbegin(); it != propValues.cend(); ++it) {
        auto array = vtkSmartPointer<vtkFloatArray>::New();
        array->SetName(it.key().toUtf8().constData());
        array->SetNumberOfTuples(points->GetNumberOfPoints());
        for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
            array->SetValue(i, it.value().at(int(i)));
        poly->GetPointData()->AddArray(array);
    }
    return poly;
}

} // namespace tsurf
