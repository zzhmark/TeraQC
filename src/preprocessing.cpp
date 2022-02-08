/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#include "preprocessing.h"
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;


/*
 * Custom Canny Edge Detection
 * This function provides for 16bit images, using opencv.
 * It's only available in opencv after 3.2. To be compatible with
 * qt-4.8.6 and msvc120, it has to be realized with functions in opencv 3.1.
 * It assumes the pixel type to be 16bit and calculate gradient using float32.
 * The thresholds are defined as ratios of the maxiumal magnitude, not fixed values.
 *
 */
void Canny16bit(InputArray in, OutputArray edges, double threshold1, double threshold2)
{
    assert(threshold1 < threshold2 && threshold1 >= 0 && threshold2 <= 1);
    int di[5] = {1, 1, 0,-1,-1};
    int dj[5] = {0,-1,-1,-1, 0};
    Mat dx, dy, phi, mag;
    Sobel(in, dx, CV_32F, 1, 0);
    Sobel(in, dy, CV_32F, 0, 1);
    magnitude(dx, dy, mag);
    phase(dx, dy, phi);
    double min, max;
    minMaxLoc(mag, &min, &max);
    auto low = max * threshold1;
    auto high = max * threshold2;
    // NMS
    Mat nms = mag.clone();
    for (int i = 1; i < mag.rows - 1; ++i)
    {
        for (int j = 1; j < mag.cols - 1; ++j)
        {
            auto g = mag.at<float>(i, j);
            if (g == 0.0f) continue;
            auto t = phi.at<float>(i, j);
            int ind;
            double k;
            if (t >= -CV_PI/2 && t < -CV_PI/4)
            {
                k = tan(t + CV_PI/2);
                ind = 0;
            }
            else if (t >= -CV_PI/4 && t < 0)
            {
                k = tan(t + CV_PI/4);
                ind = 1;
            }
            else if (t >= 0 && t < CV_PI/4)
            {
                k = tan(t);
                ind = 2;
            }
            else
            {
                k = tan(t - CV_PI/4);
                ind = 3;
            }
            auto g0u = mag.at<float>(i+di[ind+1], j+dj[ind+1]);
            auto g0d = mag.at<float>(i+di[ind], j+dj[ind]);
            auto g1u = mag.at<float>(i-di[ind+1], j-dj[ind+1]);
            auto g1d = mag.at<float>(i-di[ind], j-dj[ind]);
            auto g0 = k * (g0u - g0d) + g0d;
            auto g1 = k * (g1u - g1d) + g1d;
            if (g <= g0 || g <= g1) nms.at<float>(i, j) = 0.0f;
        }
    }

    // double threshold
    QQueue<QPoint> q;
    for (int i = 0; i < nms.rows; ++i)
    {
        for (int j = 0; j < nms.cols; ++j)
        {
            auto& x = nms.ptr<float>(i)[j];
            if (x > high)
            {
                x = FLT_MAX;
                // seeds
                q.enqueue(QPoint(i, j));
            }
            if (x < low)
                x = 0.0f;
        }
    }

    // linking
    while (!q.isEmpty())
    {
        auto h = q.dequeue();
        for (int m = -1; m <= 1; ++m)
            for (int n = -1; n <= 1; ++n)
            {
                auto i = h.y() + m;
                auto j = h.x() + n;
                if (i < 0 || i >= nms.rows || j < 0 || j >= nms.cols) continue;
                auto& x = nms.ptr<float>(i)[j];
                if (x == FLT_MAX || x == 0.0f) continue;
                x = FLT_MAX;
                q.enqueue(QPoint(i, j));
            }
    }

    // clearing
    for (int i = 0; i < nms.rows; ++i)
        for (int j = 0; j < nms.cols; ++j)
        {
            auto& x = nms.ptr<float>(i)[j];
            if (x < FLT_MAX) x = 0.0f;
        }

    convertScaleAbs(nms, edges, UCHAR_MAX / FLT_MAX);
}


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
 * extendRatio: drawn lines are extended by this ratio on both ends;
 *
 * cannyMin & cannyMax: canny thresholds, as ratios of max sobel edge gradient magnitude;
 *
 * sigma: gaussian filter param before sobel, kernel size as 3 times of this.
 *
*/

bool findMarkers(const QcImage& input, QcImage& output, const QVariantMap& params)
{
    // used params
    int se1, se2, se3, houghDistanceRes, houghAngleRes, houghThreshold,
            houghMinLineLength, houghMaxLineGap, lineWidth;
    double filterMinDistance, filterAngleLimit, zThickness, extendRatio, cannyMin, cannyMax, sigma;

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
        lineWidth = params.value("lineWidth", 3).toUInt();

        extendRatio = params.value("extendRatio", 0.1).toDouble();
        filterMinDistance = params.value("filterMinDistance", 300.0).toDouble();
        filterAngleLimit = params.value("angleLimit", 5.0).toDouble();
        zThickness = params.value("zThickness", 2.0).toDouble();
        cannyMin = params.value("cannyMin", 0.1).toDouble();
        cannyMax = params.value("cannyMax", 0.3).toDouble();
        sigma = params.value("sigma", 1.0).toDouble();
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
                cvtype = CV_8U;
                break;
            case V3D_UINT16:
                cvtype = CV_16U;
                break;
            case V3D_FLOAT32:
                cvtype = CV_32F;
                break;
            default:
                cvtype = CV_8U;

        }
        auto matInputBuffer = Mat(sz[2], sz[1] * sz[0], cvtype, (void*)input.buffer);
        auto matOutputBuffer = Mat(sz[2], sz[1] * sz[0], CV_8U, (void*)output.buffer);

        auto k1 = getStructuringElement(MORPH_ELLIPSE, Size(se1, se1));
        auto k2 = getStructuringElement(MORPH_ELLIPSE, Size(se2, se2));
        auto k3 = getStructuringElement(MORPH_RECT, Size(1, se3));
        auto gk = int(abs(sigma*3));
        if (gk % 2 == 0) ++gk;
        // iterate over all slices to do the smotthing and sobel edge detection (with OPENCV)
        for (int i = 0; i < sz[2]; ++i)
        {
            auto inputSlice = matInputBuffer.row(i).reshape(0, sz[1]);
            auto outputSlice = matOutputBuffer.row(i).reshape(0, sz[1]);
//            Mat smooth, grad_x, grad_y, edges, canny;
            Mat smooth, edges;
            morphologyEx(inputSlice, smooth, MORPH_CLOSE, k1);
            GaussianBlur(smooth, smooth, Size(gk, gk), sigma);
            // the canny function is only available after opencv 3.2
            // so use the DIY canny instead
//            Sobel(smooth, grad_x, CV_16S, 1, 0);
//            Sobel(smooth, grad_y, CV_16S, 0, 1);
//            magnitude(grad_x, grad_y, edges);
//            double min, max;
//            minMaxLoc(edges, &min, &max);
//            Canny(grad_x, grad_y, edges, cannyMin * max, cannyMax * max, true);
            Canny16bit(smooth, edges, cannyMin, cannyMax);
            morphologyEx(edges, edges, MORPH_CLOSE, k2);
            vector<Vec4i> lines;
            HoughLinesP(edges, lines, houghDistanceRes, M_PI / houghAngleRes,
                        houghThreshold, houghMinLineLength, houghMaxLineGap);

            outputSlice = 0;
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
                    line(outputSlice, Point(p1.x(), p1.y()), Point(p2.x(), p2.y()), UCHAR_MAX, lineWidth);
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
                cvtype = CV_8U;
                break;
            case V3D_UINT16:
                cvtype = CV_16U;
                break;
            case V3D_FLOAT32:
                cvtype = CV_32F;
                break;
            default:
                cvtype = CV_8U;
        }
        auto matInputBuffer = Mat(sz[2], sz[1] * sz[0], cvtype, (void*)input.buffer);
        auto matOutputBuffer = Mat(sz[2], sz[1] * sz[0], cvtype, (void*)output.buffer);
        auto matMaskBuffer = Mat(sz[2], sz[1] * sz[0], CV_8U, (void*)mask.buffer);
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
