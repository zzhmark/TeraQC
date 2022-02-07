/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#include "preprocessing.h"
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

/*
 * Test if the line meet specified standards
 *
 * Hough transform can detect some artifacts as lines, thus we can
 * use some spatial info, including lines' orientation and distance to
 * image center, to further filter these lines.
 *
 * Params:
 *
 * line: the candidate line as 4 intergers;
 *
 * layer: the layer of the stack, used to calculate 3D distance;
 *
 * minDist: minimal 3D distance to the image center;
 *
 * angleLim: limit of the line's angle with horizontal and vertical directions;
 *
 * size: size of the 3D image;
 *
 * zThickness: scaling factor to calculate z-axis length.
 *
 */

bool testLine(const Vec4i& line,
              int layer,
              double minDist,
              double angleLim,
              const QVector3D& size,
              double zThickness)
{
    auto center = size / 2 * QVector3D(1, 1, zThickness);
    auto qline = QLineF(line[0], line[1], line[2], line[3]);

    // test if the line's distance to image's center is greater than the threshold
    auto testDistance = [&](const QLineF& line)
    {
        auto p1 = QVector3D(line.p1()), p2 = QVector3D(line.p2());
        p1.setZ(layer * zThickness);
        p2.setZ(layer * zThickness);
        return QVector3D::crossProduct(p2 - center, p1 - center).length() / (p2 - p1).length() >= minDist;
    };

    // test if the line's orientation is horizontal or vertical (within threshold in degrees)
    auto testAngle = [&](const QLineF& line)
    {
        if (size.x() > size.y())
            return abs(line.angleTo(QLineF(0, 1, 0, 0))) <= angleLim ||
                    abs(line.angleTo(QLineF(0, -1, 0, 0))) <= angleLim;
        else
            return abs(line.angleTo(QLineF(1, 0, 0, 0))) <= angleLim ||
                    abs(line.angleTo(QLineF(-1, 0, 0, 0))) <= angleLim;
    };

    return testDistance(qline) && testAngle(qline);
}


/*
 * Compute mask for markers
 *
 * Algorithm:
 *
 * 1. Grayscale morphological closing to smoothing the image (reduce edges);
 *
 * 2. Sobel first order edge detection;
 *
 * 3. OTSU (stack level) to get all prominent edges;
 *
 * 4. Morphological closing (filling holes);
 *
 * 5. Hough Lines Probability Transform;
 *
 * 6. Filter lines based on orientation and distance to the 3D image center;
 *
 * 7. Draw lines;
 *
 * 8. z-axis wise interpolation using morphological closing.
 *
 * Params:
 *
 * se1: smoothing kernel size;
 *
 * se2: filling hole kernel size;
 *
 * se3: interpolation kernel size;
 *
 * houghDistanceRes: length resolution of the hough transform;
 *
 * houghAngleRes: angle resolution of the hough transform, defined as number of divisions of a circle;
 *
 * houghThreshold: the minimum vote for a line in the hough transform;
 *
 * houghMinLineLength: the minimal line length detect by hough transform;
 *
 * houghMaxLineGap: the maximal gap between points in a line to be detected in hough transform;
 *
 * filterMinDist: the minimal distance allowed between the marker line and the center of the 3D image;
 *
 * zThickness: the z-axis scaling factor used in calculating distance;
 *
 * filterAngleLimit: the maximal angle of marker lines to x or y axis;
 *
 * lineWidth: the lineWidth to draw markers in the mask;
 *
 * extendRatio: drawn lines are extended by this ratio on both ends.
 *
*/

bool findMarkers(const QcImage& input, QcImage& output, const QVariantMap& params)
{
    // used params
    int se1, se2, se3, houghDistanceRes, houghAngleRes, houghThreshold,
            houghMinLineLength, houghMaxLineGap, lineWidth;
    double filterMinDistance, filterAngleLimit, zThickness, extendRatio;

    // argument parsing
    try {
        se1 = params.value("se1", 11).toUInt();
        se2 = params.value("se1", 5).toUInt();
        se3 = params.value("se1", 21).toUInt();
        houghDistanceRes = params.value("houghDistanceRes", 1).toUInt();
        houghAngleRes = params.value("houghAngleRes", 180).toUInt();
        houghThreshold = params.value("houghThreshold", 100).toUInt();
        houghMinLineLength = params.value("houghMinLineLength", 100).toUInt();
        houghMaxLineGap = params.value("houghMaxLineGap", 1).toUInt();
        lineWidth = params.value("lineWidth", 5).toUInt();

        extendRatio = params.value("extendRatio", 0.1).toDouble();
        filterMinDistance = params.value("filterMinDistance", 300.0).toDouble();
        filterAngleLimit = params.value("angleLimit", 10.0).toDouble();
        zThickness = params.value("zThickness", 3.0).toDouble();
    }  catch (...) {
        cerr << "Argument Parsing Error. Please check the argument list." << endl;
        return false;
    }

    // image processing
    try
    {
        const auto& sz = input.sz;
        output.clear();
        output.create(sz, V3D_UINT8);

        // use buffer as an opencv accessor
        int cvtype;
        switch (input.datatype)
        {
            case V3D_UINT8:
                cvtype = CV_8UC1;
                break;
            case V3D_UINT16:
                cvtype = CV_16UC1;
                break;
            case V3D_FLOAT32:
                cvtype = CV_32FC1;
                break;
            default:
                cvtype = CV_8UC1;

        }
        auto matInputBuffer = Mat(sz[2], sz[1] * sz[0], cvtype, (void*)input.buffer);
        auto matOutputBuffer = Mat(sz[2], sz[1] * sz[0], CV_8UC1, (void*)output.buffer);

        auto k1 = getStructuringElement(MORPH_ELLIPSE, Size(se1, se1));
        auto k2 = getStructuringElement(MORPH_ELLIPSE, Size(se2, se2));
        auto k3 = getStructuringElement(MORPH_RECT, Size(1, se3));
        // iterate over all slices to do the smotthing and sobel edge detection (with OPENCV)
        for (int i = 0; i < sz[2]; ++i)
        {
            auto inputSlice = matInputBuffer.row(i).reshape(0, sz[1]);
            auto outputSlice = matOutputBuffer.row(i).reshape(0, sz[1]);
            Mat closed, grad_x, grad_y, edges;
            morphologyEx(inputSlice, closed, MORPH_CLOSE, k1);
            Sobel(closed, grad_x, CV_32F, 1, 0);
            Sobel(closed, grad_y, CV_32F, 0, 1);
            magnitude(grad_x, grad_y, edges);
            double min, max;
            minMaxLoc(edges, &min, &max);
            convertScaleAbs(edges, outputSlice, 255 / max);

        }

        // otsu
        threshold(matOutputBuffer, matOutputBuffer, 0, 255, THRESH_OTSU);
        // iterate again to compute hough transform and draw masks
        for (int i = 0; i < sz[2]; ++i)
        {
            auto slice = matOutputBuffer.row(i).reshape(0, sz[1]);
            vector<Vec4i> lines;
            HoughLinesP(slice, lines, houghDistanceRes, M_PI / houghAngleRes,
                        houghThreshold, houghMinLineLength, houghMaxLineGap);
            slice = 0;
            for (int j = 0; j < lines.size(); ++j)
                if (testLine(lines[j], i, filterMinDistance, filterAngleLimit,
                             QVector3D(sz[0], sz[1], sz[2]), zThickness))
                {
                    // lengthen
                    auto p1 = QVector2D(lines[j][0], lines[j][1]);
                    auto p2 = QVector2D(lines[j][2], lines[j][3]);
                    auto d = p1 - p2;
                    p1 = p1 + d * extendRatio;
                    p2 = p2 - d * extendRatio;
                    // draw
                    line(slice, Point(p1.x(), p1.y()), Point(p2.x(), p2.y()), 255, lineWidth);
                }

        }

        // z interpolation
        morphologyEx(matOutputBuffer, matOutputBuffer, MORPH_CLOSE, k3);

        return true;
    }
    catch(...)
    {
        cerr << "ERROR: Unkown exception, probably related to OPENCV functions." << endl;
        output.clear();
        return false;
    }
}

bool masking(const QcImage& input, QcImage& output, const QcImage& mask)
{
    try
    {
        const auto& sz = input.sz;
        output.clear();
        output.create(sz, input.datatype);
        int cvtype;
        switch (input.datatype)
        {
            case V3D_UINT8:
                cvtype = CV_8UC1;
                break;
            case V3D_UINT16:
                cvtype = CV_16UC1;
                break;
            case V3D_FLOAT32:
                cvtype = CV_32FC1;
                break;
            default:
                cvtype = CV_8UC1;

        }
        auto matInputBuffer = Mat(sz[2], sz[1] * sz[0], cvtype, (void*)input.buffer);
        auto matOutputBuffer = Mat(sz[2], sz[1] * sz[0], cvtype, (void*)output.buffer);
        auto matMaskBuffer = Mat(sz[2], sz[1] * sz[0], CV_8UC1, (void*)mask.buffer);
        bitwise_and(matInputBuffer, matInputBuffer, matOutputBuffer, matMaskBuffer);
        return true;
    }
    catch(...)
    {
        cerr << "ERROR: Unkown exception, probably related to OPENCV functions." << endl;
        output.clear();
        return false;
    }
}
