#include "rasterize.h"
#include "input.h"
#include <opencv2/opencv.hpp>

void ici::rasterize(const std::vector<circle>& circles, const input& inp)
{
    int width = 640;
    int height = 480;

    cv::Mat image(height, width, CV_8UC3, cv::Scalar(0, 0, 0));

    cv::Point center(width / 2, height / 2);
    int radius = 100;

    cv::Scalar color(0, 255, 0); // Green color

    // Define the thickness of the circle border (negative value means filled circle)
    int thickness = -1; // Filled circle

    // Draw the circle onto the image
    cv::circle(image, center, radius, color, thickness);

    // Save the image to the specified path
    cv::imwrite(inp.out_file, image);
}
