

#include "my_algos/filters.h"
#include "my_algos/basics.h"
#include <iostream>
#include <cmath>
namespace filters
{
// typedef unsigned char uchar;

/**
 * Filter the input gray image with kernel. Output float image.
 */
cv::Mat1f conv2D(const cv::Mat &src, const Kernel &kernel)
{

    assert(src.channels() == 1);
    assert(kernel.size() % 2 == 1 && kernel[0].size() % 2 == 1);

    cv::Mat src_32FC1 = src;
    if (src.depth() != CV_32FC1)
        src.convertTo(src_32FC1, CV_32FC1);

    const int r1 = kernel.size() / 2;    // Radius of the kernel.
    const int r2 = kernel[0].size() / 2; // Radius of the kernel.

    cv::Mat1f dst = cv::Mat::zeros(src.rows, src.cols, CV_32FC1); // Pad zeros.
    for (int i = r1; i < src.rows - r1; ++i)
        for (int j = r2; j < src.cols - r2; ++j)
        {
            float sums = 0;
            for (int m = -r1; m <= r1; m++)
                for (int n = -r2; n <= r2; n++)
                    sums += src_32FC1.at<float>(i + m, j + n) * kernel[m + r1][n + r2];
            dst.at<float>(i, j) = sums;
        }
    return dst;
}

cv::Mat1f sobelX(const cv::Mat1b &src)
{
    // https://en.wikipedia.org/wiki/Sobel_operator
    const Kernel kernel = {{-1., 0., 1.},
                           {-2., 0., 2.},
                           {-1., 0., 1.}};
    const Kernel sub_kernel_1 = {{-1., 0., 1.}};                     // Horizontal.
    const Kernel sub_kernel_2 = {{1.}, {2.}, {1.}};                  // Vertical.
    cv::Mat1f dst = conv2D(conv2D(src, sub_kernel_1), sub_kernel_2); // Faster.
    // cv::Mat1f dst = conv2D(src, kernel);
    return dst;
}

cv::Mat1f sobelY(const cv::Mat1b &src)
{
    const Kernel kernel = {{-1., -2., -1.},
                           {0., 0., 0.},
                           {1., 2., 1.}};
    // https://en.wikipedia.org/wiki/Sobel_operator
    const Kernel sub_kernel_1 = {{1., 2., 1.}};                      // Horizontal.
    const Kernel sub_kernel_2 = {{-1.}, {0.}, {1.}};                 // Vertical.
    cv::Mat1f dst = conv2D(conv2D(src, sub_kernel_1), sub_kernel_2); // Faster.
    // cv::Mat1f dst = conv2D(src, kernel);
    return dst;
}

cv::Mat1f sobel(const cv::Mat1b &src)
{
    cv::Mat1f sobel_x = sobelX(src);
    cv::Mat1f sobel_y = sobelY(src);
    cv::Mat1f dst_Ig = cv::Mat::zeros(src.size(), CV_32FC1);
    for (int i = 0; i < src.rows; ++i)
        for (int j = 0; j < src.cols; ++j)
            dst_Ig.at<float>(i, j) = sqrt(
                pow(sobel_x.at<float>(i, j), 2.) +
                pow(sobel_y.at<float>(i, j), 2.));
    return dst_Ig;
}

/**
 * Canny edge detection.
 * Algorithm: https://docs.opencv.org/3.4/da/d5c/tutorial_canny_detector.html
 * Procedures:
 *      1. Compute image gradient Ix, Iy.
 *      2. Compute gradient's magnitude Ig and direction Id.
 *      3. Non maximum suppression along the gradient direction and update Ig.
 *      4. Mask pixels (i, j) as edge if Ig[i, j] >= ub.
 *      5. Starting from previous edge pixels, 
 *          do deep first search to find the connected pixels
 *          whose gradient is larger than lb.
 */
cv::Mat1b canny(const cv::Mat1b &src, const float lb, const float ub)
{

    const int r1 = 1; // radius of the gaussian filter.
    cv::Mat src_blurred = conv2D(src, kernels::GAUSSION_3x3);
    src_blurred.convertTo(src_blurred, CV_8UC1); // float to uchar.

    const int r2 = 1;                   // radius of the sobel kernel.
    cv::Mat1f Ix = sobelX(src_blurred); // Gradient x.
    cv::Mat1f Iy = sobelY(src_blurred); // Gradient y.

    const int r = r1 + r2; // Total offset.

    // -- Step 2: Compute gradient's magnitude Ig and direction Id.
    cv::Mat1f Ig = cv::Mat::zeros(src.size(), CV_32FC1); // Maginitude.
    cv::Mat1f Id = cv::Mat::zeros(src.size(), CV_32FC1); // Direction.
    for (int i = r; i < src.rows - r; ++i)
        for (int j = r; j < src.cols - r; ++j)
        {
            float dx = Ix.at<float>(i, j);
            float dy = Iy.at<float>(i, j);
            Ig.at<float>(i, j) = sqrt(pow(dx, 2.) + pow(dy, 2.));
            Id.at<float>(i, j) = atan2(dy, dx);
        }

    // -- Step 3: Non maximum suppression along the gradient direction.
    cv::Mat1f Ig_tmp = Ig.clone();
    const std::vector<cv::Point2i> row_col_offsets = {
        {0, 1},
        {1, 1},
        {1, 0},
        {1, -1},
        {0, -1},
        {-1, -1},
        {-1, 0},
        {-1, 1}};
    for (int i = r; i < src.rows - r - r; ++i)
        for (int j = r; j < src.cols; ++j)
        {
            int index = abs(lround(Id.at<float>(i, j) / M_PI_4));
            assert(index <= 4);
            int drow = row_col_offsets.at(index).x, dcol = row_col_offsets.at(index).y;
            int r1 = i + drow, r2 = i - drow;
            int c1 = j + dcol, c2 = j - dcol;
            if (Ig.at<float>(i, j) < Ig.at<float>(r1, c1) ||
                Ig.at<float>(i, j) < Ig.at<float>(r2, c2))
                Ig_tmp.at<float>(i, j) = 0.;
        }
    Ig = Ig_tmp;

    // -- Mask pixels (i, j) as edge if Ig[i, j] >= ub.
    cv::Mat1b res_mask = cv::Mat::zeros(src.size(), CV_8U);
    for (int i = 0; i < src.rows; ++i)
        for (int j = 0; j < src.cols; ++j)
        {
            if (Ig.at<float>(i, j) > ub)
                res_mask.at<uchar>(i, j) = 255;
        }

    return res_mask;
}

// /**
//  * Compute sobel gradient.
//  */
// cv::Mat1b sobel(const cv::Mat1b &src, int low_thresh, int high_thresh){
//     const int KERNEL_X[] = {-1,     }
} // namespace filters
