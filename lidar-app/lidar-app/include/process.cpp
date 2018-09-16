#include "process.hpp"

Process::Process()
{
    gap_pos_left = cv::Point(100, 100);
    gap_pos_right = cv::Point(200, 200);
}

void Process::simplify_data(std::vector<cv::Point> &points)
{
    cv::Point tmp, last;
    std::vector<cv::Point> tmp_vec;

    tmp_vec.push_back(points[0]);
    last = points[0];

    for(uint32_t i = 1; i < points.size() - 1; i++)
    {
        tmp = points[i] - last;
        if(sqrt(tmp.x*tmp.x + tmp.y*tmp.y) > thresh_simplify)
        {
            tmp_vec.push_back(points[i]);
            last = points[i];
        }
    }

    points.clear();
    points = tmp_vec;
}

void Process::split_poins(std::vector<cv::Point> &points)
{
    left_points.clear();
    right_points.clear();
    rejected_points.clear();

    uint32_t i;
    uint32_t j;

    if(points.size() >= 4)
    {
        left_points.push_back(points[points.size()-1]);

        for(i = points.size()-2; i > 0; i--)
        {
            if(sqrt((points[i].x - points[i+1].x)*(points[i].x - points[i+1].x) + (points[i].y - points[i+1].y)*(points[i].y - points[i+1].y)) < max_dist)
                left_points.push_back(points[i]);
            else
                break;
        }

        right_points.push_back(points[0]);

        for(j = 1; j < points.size(); j++)
        {
            if(sqrt((points[j].x - points[j-1].x)*(points[j].x - points[j-1].x) + (points[j].y - points[j-1].y)*(points[j].y - points[j-1].y)) < max_dist)
                right_points.push_back(points[j]);
            else
                break;
        }

        int i_int = i;
        int j_int= j;

        if(i_int - j_int >= 0)
        {
            for(uint32_t k = j; k <= i; k++)
                rejected_points.push_back(points[k]);
        }
    }

}

void Process::draw_data(cv::Mat &out)
{
#ifdef VERBOSE_MODE
    std::cout << "Left:     " << left_points.size() << std::endl;
    std::cout << "Right:    " << right_points.size() << std::endl;
    std::cout << "Rejected: " << rejected_points.size() << std::endl;
#endif

    for(uint32_t i = 0; i < left_points.size(); i++)
        cv::circle(out, left_points[i], 2, cv::Scalar(0, 0, 255), 2, 8, 0);

    for(uint32_t i = 0; i < right_points.size(); i++)
        cv::circle(out, right_points[i], 2, cv::Scalar(0, 255, 0), 2, 8, 0);

    for(uint32_t i = 0; i < rejected_points.size(); i++)
        cv::circle(out, rejected_points[i], 2, cv::Scalar(255, 0, 0), 2, 8, 0);
}