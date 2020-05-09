#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <chrono>
#include <cmath>
#include <random>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>

#include "delaunator-header-only.hpp"
#include <vtkTriangle.h>
#include <vtkCellArray.h>

#include <vtkDelaunay2D.h>

void fasterDelaunayTriangulation(vtkSmartPointer<vtkPolyData> polydata) {
    const size_t number_of_points = polydata->GetNumberOfPoints();
    std::vector<double> coords(number_of_points * 2);

    for (size_t i = 0; i < number_of_points; ++i) {
        std::array<double, 3> p;
        polydata->GetPoint(i, p.data());
        coords[i * 2 + 0] = p[0];
        coords[i * 2 + 1] = p[1];
    }

    delaunator::Delaunator d(coords);
 
    vtkNew<vtkCellArray> triangles;
    for (size_t i = 0; i < d.triangles.size(); i+=3) {
        vtkNew<vtkTriangle> triangle;
        triangle->GetPointIds()->SetId(0, d.triangles[i + 0]);
        triangle->GetPointIds()->SetId(1, d.triangles[i + 1]);
        triangle->GetPointIds()->SetId(2, d.triangles[i + 2]);
        triangles->InsertNextCell(triangle);
    }

    polydata->SetPolys(triangles);
}

void vtkDelaunayTriangulation(vtkSmartPointer<vtkPolyData> polydata) {
    vtkNew<vtkDelaunay2D> delaunay;
    delaunay->SetInputData(polydata);
    delaunay->Update();
    polydata->DeepCopy(delaunay->GetOutput());
}

void createPoints(vtkSmartPointer<vtkPolyData> polydata, const size_t line_size) {
    const double line_length = 5.0;
    const size_t line_num    = 10;

    std::mt19937 mt(12345);
    std::uniform_real_distribution<> dist(-line_length / 100, line_length / 100);
    std::vector<double> coords;

    vtkNew<vtkPoints> points;
    for (size_t i = 0; i < line_num; ++i) {
        const double angle = 2 * M_PI / line_num * i;
        for (size_t j = 0; j < line_size; ++j) {
            const double temp_x = line_length / line_size * j - line_length / 2;
            const double x = std::cos(angle) * temp_x;
            const double y = std::sin(angle) * temp_x;
            points->InsertNextPoint(x, y, dist(mt));
        }
    }

    polydata->SetPoints(points);
}

void calcDelaunay(vtkSmartPointer<vtkPolyData> polydata, const size_t line_size, const std::string type) {
    createPoints(polydata, line_size);

    if (type == "fast") {
        fasterDelaunayTriangulation(polydata);
    }
    else {
        vtkDelaunayTriangulation(polydata);
    }
}

void writeResult(vtkSmartPointer<vtkPolyData> src, const double elapsed, const std::string type) {
    const size_t number_of_points = src->GetNumberOfPoints();
    std::stringstream ss;
    ss << number_of_points << "points, " << elapsed << "[s]";

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(src);

    vtkNew<vtkActor> actor;
    actor->GetProperty()->SetRepresentationToWireframe();
    actor->GetProperty()->ShadingOff();
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> renderer;
    renderer->AddActor(actor);

    vtkNew<vtkTextActor> text_actor;
    text_actor->SetInput(ss.str().c_str());
    text_actor->SetPosition2(10, 40);
    text_actor->GetTextProperty()->SetFontSize(24);
    text_actor->GetTextProperty()->SetColor(1.0, 1.0, 1.0);

    vtkNew<vtkRenderWindow> render_window;
    render_window->SetSize(600, 600);
    render_window->AddRenderer(renderer);
    renderer->AddActor2D(text_actor);

    render_window->Render();

    vtkNew<vtkWindowToImageFilter> filter;
    filter->SetInput(render_window);
    filter->Update();

    std::string filename = std::to_string(number_of_points) + "_" + type + ".png";
    vtkNew<vtkPNGWriter> writer;
    writer->SetFileName(filename.c_str());
    writer->SetInputData(filter->GetOutput());
    writer->Write();
}

int main() {
    std::array<std::string, 2> types = {"fast", "conv"};

    for (const std::string& type : types) {
        for (int i = 1; i < 6; ++i) {
            std::chrono::system_clock::time_point start, end;
            start = std::chrono::system_clock::now();

            vtkNew<vtkPolyData> polydata;
            calcDelaunay(polydata, std::pow(10, i), type);

            end = std::chrono::system_clock::now();
            const double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;

            std::cout << type << " " << i << ": " << elapsed << "[s]" << std::endl;

            writeResult(polydata, elapsed, type);
        }
    }

    return 0;
}