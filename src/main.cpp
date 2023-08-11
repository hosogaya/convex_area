#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/highgui/highgui_c.h>
#include <iostream>
#include <vector>
#include <numeric>
#include <random>

struct Vertex 
{
    bool vaild = false;
    double losing_area = 0.0;
    cv::Point point;
};

int circleIndex(const int index, const int size)
{
    if (index >= 0 && index < size) return index;
    else if (index < 0) return size + index;
    else if (index >= size) return index - size;
}

double calCross(const cv::Point& last, const cv::Point& vertex, const cv::Point& end)
{
    cv::Point v1 = vertex - last;
    cv::Point v2 = end - vertex;
    return v1.x*v2.y - v1.y*v2.x;
}

int findConvexVertex(std::vector<Vertex>& vertex, const int start_index, const bool c_wise)
{
    std::cout << "start index: " << start_index << std::endl;
    std::cout << "vertex size: " << vertex.size() << std::endl;
    std::cout << "thres: " << start_index - static_cast<int>(vertex.size()) << std::endl;
    if (c_wise)
    {
        std::cout << "Clock wise" << std::endl;
        for (int i=start_index; i>-static_cast<int>(vertex.size() - start_index); --i)
        {
            int index = circleIndex(i, vertex.size());
            std::cout << i << " index: " << index << std::endl;
            if (vertex[index].vaild) 
            {
                return index;
            }
        }
    }
    else 
    {
        std::cout << "Counter clock wise" << std::endl;
        for (int i=start_index; i<vertex.size()+start_index; ++i)
        {
            int index = circleIndex(i, vertex.size());
            std::cout << i << " index: " << index << std::endl;
            if (vertex[index].vaild) 
            {
                return index;
            }
        }
    }
    return -1; // Dose not exist converx vertex
}

void updateVertex(std::vector<Vertex>& vertex, const int remove_index)
{
    
    // update info
    int index1 = circleIndex(remove_index-1, vertex.size());
    double cross1 = calCross(vertex[circleIndex(remove_index-2, vertex.size())].point, vertex[index1].point, vertex[circleIndex(remove_index+1, vertex.size())].point);
    if (cross1 >= 0.0) vertex[index1].vaild = false;
    else vertex[index1].vaild = true;
    
    int index2 = circleIndex(remove_index+1, vertex.size());
    double cross2 = calCross(vertex[circleIndex(remove_index-1, vertex.size())].point, vertex[index2].point, vertex[circleIndex(remove_index+2, vertex.size())].point);
    if (cross2 >= 0.0) vertex[index2].vaild = false;
    else vertex[index2].vaild = true;
}

void removeVertex(std::vector<Vertex>& vertex, const int non_convex_index)
{
    if (vertex.size() == 3) return;
    if (vertex.size() == 4)
    {
        std::vector<cv::Point> contour1{vertex[circleIndex(non_convex_index, vertex.size())].point, 
                                       vertex[circleIndex(non_convex_index+1, vertex.size())].point, 
                                       vertex[circleIndex(non_convex_index+2, vertex.size())].point};
        double area1 = cv::contourArea(contour1, false);
        
        std::vector<cv::Point> contour2{vertex[circleIndex(non_convex_index, vertex.size())].point, 
                                       vertex[circleIndex(non_convex_index-1, vertex.size())].point, 
                                       vertex[circleIndex(non_convex_index-2, vertex.size())].point};
        double area2 = cv::contourArea(contour2, false);
        std::cout << "Area1: " << area1 << " Area2: " << area2 << std::endl;
        if (area1 < area2) 
        {
            int index = findConvexVertex(vertex, circleIndex(non_convex_index, vertex.size()), false);
            if (index>=0) 
            {
                updateVertex(vertex, index);
                vertex.erase(vertex.begin() + index);
            }
        }
        else 
        {
            int index = findConvexVertex(vertex, circleIndex(non_convex_index, vertex.size()), true);
            if (index>=0) 
            {
                updateVertex(vertex, index);
                vertex.erase(vertex.begin() + index);
            }
        }
        return;
    }

    // Counter-clock wise 
    int cc_index = findConvexVertex(vertex, non_convex_index, false);
    if (cc_index >= 0) updateVertex(vertex, cc_index);
    // Clock wise
    int c_index = findConvexVertex(vertex, non_convex_index, true);
    if (c_index >= 0) updateVertex(vertex, c_index);

    if (cc_index<0 && c_index<0) return;
    else if (cc_index<0 && c_index>=0) vertex.erase(vertex.erase(vertex.begin() + c_index));
    else if (cc_index>=0 && c_index<0) vertex.erase(vertex.erase(vertex.begin() + cc_index));
    else 
    {
        if (c_index > cc_index) 
        {
            vertex.erase(vertex.begin() + c_index);
            vertex.erase(vertex.begin() + cc_index);
        }
        else if (cc_index > c_index)
        {
            vertex.erase(vertex.begin() + cc_index);
            vertex.erase(vertex.begin() + c_index);
        }
        else 
        {
            vertex.erase(vertex.begin() + c_index);
        }
    }

}

int main()
{
    // Read source image
    std::string file_name = "star2.jpeg";
    cv::Mat image = cv::imread(RESOURCE_DIR+file_name);
    if (image.empty()) 
    {
        std::cout << "Error: Cannot find image file: " <<  RESOURCE_DIR+file_name << std::endl;
        return 1;
    }
 
    // Transform to gray scale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::blur(gray, gray, cv::Size(5, 5));

    // Transform to binary image
    cv::Mat binary;
    cv::threshold(gray, binary, 200, 255, CV_THRESH_BINARY);
    cv::bitwise_not(binary, binary);

    // Find contours
    std::vector<std::vector<cv::Point>> src_contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(binary, src_contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_TC89_KCOS);

    // Check convexity
    std::vector<std::vector<cv::Point>> hull(src_contours.size());
    for (int i=0; i<src_contours.size(); ++i)
    {
        cv::convexHull(src_contours[i], hull[i]);
    }

    // calculate losing area
    std::vector<std::vector<cv::Point>> contours = src_contours;
    // std::vector<std::vector<cv::Point>> contours(1);
    // contours[0].resize(4);
    // contours[0][0] = cv::Point(100, 50);
    // contours[0][1] = cv::Point(50, 100);
    // contours[0][3] = cv::Point(120, 75);
    // contours[0][2] = cv::Point(100, 75);
    for (int i=0; i<contours.size(); ++i)
    {
        if (contours[i].size() == 3) 
        {
            // convex
            continue;
        }
        std::cout << "Initialize vertex" << std::endl;
        std::vector<Vertex> vertex(contours[i].size());
        std::cout << "Contour size: " << contours[i].size() << std::endl;
        std::cout << "Vertex size: " << vertex.size() << std::endl;
        for (int j=0; j<contours[i].size(); ++j)
        {
            double cross = calCross(contours[i][circleIndex(j-1, contours[i].size())], contours[i][circleIndex(j, contours[i].size())], contours[i][circleIndex(j+1, contours[i].size())]);
            if (cross >= 0.0)
            {
                // calculate losing area
                std::vector<cv::Point> con{contours[i][circleIndex(j, contours[i].size())], contours[i][circleIndex(j+1, contours[i].size())], contours[i][circleIndex(j+2, contours[i].size())]};
                auto& v = vertex[circleIndex(j, contours[i].size())];
                v.losing_area = cv::contourArea(con, false);
                v.vaild = false;
            }
            else
            {
                vertex[circleIndex(j, contours[i].size())].vaild = true;
            }
            int index = circleIndex(j, contours[i].size());
            vertex[index].point = contours[i][index];
        }

        std::cout << "Removing point while the polygon is non-convex" << std::endl;
        bool convex = false;
        std::vector<int> random_index(vertex.size());
        
        std::random_device rd;
        std::mt19937 g(rd());
        while (!convex)
        {
            random_index.resize(vertex.size());
            std::iota(random_index.begin(), random_index.end(), 0);
            std::shuffle(random_index.begin(), random_index.end(), g);
            convex = true;
            for (int non_convex_index: random_index)
            {
                if (non_convex_index >= static_cast<int>(vertex.size())) continue;
                if (!vertex[non_convex_index].vaild) 
                {
                    // remove
                    convex = false;
                    removeVertex(vertex, non_convex_index);
                }
            }
            std::cout << "vaild: ";
            for (int j=0; j<vertex.size(); ++j) std::cout << vertex[j].vaild << ", ";
            std::cout << std::endl;
        }
        contours[i].resize(vertex.size());
        for (int j=0; j<vertex.size(); ++j)
        {
            contours[i][j] = vertex[j].point;
        }
        std::cout << "Contour size: " << contours[i].size() << std::endl;
    }

    // Draw contours and convex area
    cv::Mat draw = cv::Mat::zeros(image.size(), CV_8UC3);
    for (int i=0; i<contours.size(); ++i)
    {
        cv::Scalar color(0, 0, 255);
        cv::drawContours(image, contours, i, color);
        cv::drawContours(image, hull, i, color);
        std::cout << "contour: ";
        for (int j=0; j<contours[i].size(); ++j)
        {
            std::cout << "[" << contours[i][j].x << ", " << contours[i][j].y << "], ";
        }
        std::cout << std::endl;
        std::cout << "cross: ";
        for (int j=1; j<contours[i].size(); ++j)
        {
            std::cout << calCross(contours[i][j-1], contours[i][j], contours[i][j+1]) << ", ";
        }
        std::cout << std::endl;
    }

    cv::imshow("star", image);

    cv::waitKey(0);
    
    return 0;
}