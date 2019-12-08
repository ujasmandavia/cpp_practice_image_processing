#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "my_algos/cv_basics.h"
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace geometry
{

/**
 * 2D line represented in polar coordinate.
 */
struct Line2d
{
    double distance; // Distance to the origin.
    double angle;    // Angle of the line in degree.
    Line2d(double distance = 0.0, double angle = 0.0) : distance(distance), angle(angle) {}
};

/**
 * Detect lines by Hough Line detection algorithm.
 * @param edge Image of edge. A pixel is edge if it has a non-zero value.
 * @param dst_polar Image of Hough Transform's result. 
 *      Row number is 180, representing 0~179 degrees; 
 *      Column number is the diagonal length of the input image. 
 * @param nms_min_pts Min points on a line. 
 * @param nms_radius Radius of doing non-maximum suppression.
 * @return Parameters of each detected line.
 */
std::vector<Line2d> houghLine(
    const cv::Mat1b &edge,
    cv::Mat1i *dst_polar = nullptr,
    int nms_min_pts = 30,
    int nms_radius = 10);

/**
 * Non-maximum suppression (NMS).
 * @param heaptmap An image that we want to find its local peaks.
 * @param radius Radius of NMS.
 * @return (x, y) position of each peak point in heatmap.
 *  The points are sort from high score to low score.
 */
template <typename pixel_type>
std::vector<cv::Point2i> nms(
    const cv::Mat &heatmap,
    const int min_value,
    const int radius)
{
    assert(heatmap.channels() == 1);

    // Detect local max and store the (score, position).
    cv::Mat mask = cv::Mat::ones(heatmap.size(), CV_8UC1);
    std::vector<std::pair<pixel_type, cv::Point2i>> peaks; // vector of (score, position).
    for (int i = 0; i < heatmap.rows; i++)
        for (int j = 0; j < heatmap.cols; j++)
        {
            const pixel_type score = heatmap.at<pixel_type>(i, j);
            if (mask.at<uchar>(i, j) == 0 || score < min_value)
                continue;
            const bool is_max = cv_basics::isLocalMax<pixel_type>(heatmap, i, j, radius);
            if (is_max)
            {
                cv_basics::setNeighborsToZero<uchar>(&mask, i, j, radius);
                peaks.push_back({score, {j, i}});
            }
        }

    // Sort peaks based on their scores.
    std::sort(peaks.begin(), peaks.end(),
              [](auto const &p1, auto const &p2) { return p1.first > p2.first; });

    // Return the positions of peaks.
    std::vector<cv::Point2i> peaks_position;
    for (auto const &peak : peaks)
        peaks_position.push_back(peak.second);
    return peaks_position;
}

} // namespace geometry
#endif