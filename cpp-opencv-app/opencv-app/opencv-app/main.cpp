#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

#include <include/sharedmemory.hpp>
#include <include/lidar.hpp>
#include <include/lanedetector.hpp>
#include <include/stoplights.h>

// Race mode ---> fps boost
// Debug mode --> display data
//#define RACE_MODE
#define DEBUG_MODE
#define NO_USB
//#define STOPLIGHTS_MODE

#define CAMERA_INDEX 0
#define CAM_RES_X 640
#define CAM_RES_Y 480
#define FRAME_TIME 30

// Handlers for custom classes
LaneDetector laneDetector;
StopLightDetector lightDetector;
Lidar lidar;
SharedMemory shm_lidar_data(50001);
SharedMemory shm_lane_points(50002);
SharedMemory shm_usb_to_send(50003);
SharedMemory shm_watchdog(50004);

// Variables for communication between threads
bool close_app = false;
std::mutex mu;

// Function declarations
//

int main()
{
     cv::Mat test(480, 640, CV_8UC3);
#ifdef DEBUG_MODE
    //FPS
    struct timespec start, end;
    unsigned int licznik_czas = 0;
    float seconds = 0;
    float fps = 0;
#endif

    // Declaration of cv::MAT variables
    cv::Mat frame(CAM_RES_Y, CAM_RES_X, CV_8UC4);
    cv::Mat bird_eye_frame(CAM_RES_Y, CAM_RES_X, CV_8UC3);
    cv::Mat bird_eye_frame_tr(CAM_RES_Y, CAM_RES_X, CV_8UC3);
    cv::Mat vector_frame(CAM_RES_Y, CAM_RES_X, CV_8UC3);
    cv::Mat undist_frame, cameraMatrix, distCoeffs;
    cv::Mat cone_frame_out;
    cv::Mat frame_out_yellow, frame_out_white, frame_out_edge_yellow, frame_out_edge_white;

#ifdef STOPLIGHTS_MODE
    //cv::Mats used in stoplight algorythm
    cv::Mat old_frame(CAM_RES_Y,CAM_RES_X,CV_8UC1);
    cv::Mat display;
    cv::Mat diffrence(CAM_RES_Y,CAM_RES_X,CV_8UC1);
#endif

    // Declaration of std::vector variables
    std::vector<std::vector<cv::Point>> vector;

    // Camera init
    cv::VideoCapture camera;
    camera.open(CAMERA_INDEX, cv::CAP_V4L2);

    // Check if camera is opened propely
    if (!camera.isOpened())
    {
        std::cout << "Error could not open camera on port: " << CAMERA_INDEX << std::endl
                  << "Closing app!" << std::endl;

        camera.release();
        return 0;
    }
    else
    {
        // Set resolution
        camera.set(cv::CAP_PROP_FRAME_WIDTH, CAM_RES_X);
        camera.set(cv::CAP_PROP_FRAME_HEIGHT, CAM_RES_Y);

        //Set and check decoding of frames
        camera.set(cv::CAP_PROP_CONVERT_RGB, true);
        std::cout << "RGB: " << camera.get(cv::CAP_PROP_CONVERT_RGB) << std::endl;
    }

    //Read XML file
    laneDetector.UndistXML(cameraMatrix, distCoeffs);
    laneDetector.CreateTrackbars();

#ifndef NO_USB
    // Lidar communication init
    lidar.init();
    lidar.allocate_memory();
    lidar.calculate_offset();
#endif

    // Shared Memory init
    shm_lidar_data.init();
    shm_lane_points.init();
    shm_usb_to_send.init();
    shm_watchdog.init();

    //Trackbars
    //

/*
    //Read from file
    cv::Mat frame_gray(CAM_RES_Y, CAM_RES_X, CV_8UC1);
    frame_gray = cv::imread("../../data/SELFIE-example-img-prosto.png", CV_LOAD_IMAGE_GRAYSCALE);

    if(!frame_gray.data)
    {
        std::cout << "No data!" << std::endl
                  << "Closing app!" << std::endl;

        return 0;
    }
*/
#ifdef STOPLIGHTS_MODE
    camera >>frame;
    lightDetector.prepare_first_image(frame,old_frame,lightDetector.roi_number);
    cv::namedWindow("Light detection", 1);
    cv::namedWindow("Frame", 1);
#endif

    // Main loop
    while(true)
    {
#ifdef DEBUG_MODE
        //FPS
        if(licznik_czas == 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &start);
        }
        //FPS
#endif

        // Get new frame from camera
        camera >> frame;

#ifdef STOPLIGHTS_MODE
        //Test ROI on input frame
        //lightDetector.test_roi(frame,display);
        lightDetector.find_start(frame,diffrence,old_frame,lightDetector.roi_number);
#ifdef DEBUG_MODE
        if (lightDetector.start_finding ==true){
            cv::imshow("Frame",frame);
            cv::imshow("Light detection",diffrence);
            //cv::imshow("Light detection", display);
        }
        if (lightDetector.start_light == true){
            std::cout<<"START"<<std::endl;
        }
        else
            std::cout<<"WAIT"<<std::endl;

        cv::waitKey(20);
#endif
#endif
#ifndef STOPLIGHTS_MODE
        // Process frame
        laneDetector.Undist(frame, undist_frame, cameraMatrix, distCoeffs);
        laneDetector.Hsv(undist_frame, frame_out_yellow, frame_out_white, frame_out_edge_yellow, frame_out_edge_white);
        laneDetector.BirdEye(frame_out_edge_yellow, bird_eye_frame);
        laneDetector.colorTransform(bird_eye_frame, bird_eye_frame_tr);

        // Detect cones
        std::vector<cv::Point> cones_vector;
        laneDetector.ConeDetection(frame, cone_frame_out, cones_vector);

        // Detect lines
        laneDetector.detectLine(bird_eye_frame, vector);
        laneDetector.drawPoints(vector, vector_frame);

        // Push data
        std::cout << "Push" << std::endl;
        shm_lane_points.push_data(vector, vector, vector); // <-- Do zmiany na 3 różne
        std::cout << "Pull" << std::endl;
        shm_lane_points.pull_data(test);

#ifdef DEBUG_MODE
        // Display info on screen
        //cv::imshow("Camera", frame);
        //cv::imshow("UndsCamera", undist_frame);
        cv::imshow("Frame", frame);
        cv::imshow("Yellow Line", frame_out_yellow);
        cv::imshow("White Line", frame_out_white);
        cv::imshow("Yellow Edges", frame_out_edge_yellow);
        cv::imshow("BirdEyeTtransform", bird_eye_frame_tr);
        cv::imshow("BirdEye", bird_eye_frame);
        cv::imshow("Vector", vector_frame);
        cv::imshow("Cone Detect", cone_frame_out);

cv::imshow("TEST", test);
test = cv::Mat::zeros(CAM_RES_Y, CAM_RES_X, CV_8UC3);

        vector_frame = cv::Mat::zeros(CAM_RES_Y, CAM_RES_X, CV_8UC3);

        // Get input from user
        char keypressed = (char)cv::waitKey(FRAME_TIME);

        // Process given input
        if( keypressed == 27 )
            break;

        switch(keypressed)
        {
        case '1':
        {
            cv::Mat LIDAR_data = cv::Mat(FRAME_X, FRAME_Y, CV_8UC3);

            while(true)
            {
                lidar.read_new_data();
                lidar.polar_to_cartesian();
                lidar.draw_data(LIDAR_data);

                // Show data in window
                cv::imshow("LIDAR", LIDAR_data);
                // Clear window for next frame data
                LIDAR_data = cv::Mat::zeros(FRAME_X, FRAME_Y, CV_8UC3);

                // Get input from user
                char keypressed_1 = (char)cv::waitKey(100);

                // Process given input
                if( keypressed_1 == 27 )
                    break;
            }


            break;
        }
        case '2':

            break;

        default:

            break;
        }

        if(licznik_czas > 1000)
        {
            licznik_czas = 0;
            clock_gettime(CLOCK_MONOTONIC, &end);
            seconds = (end.tv_sec - start.tv_sec);
            fps  =  1 / (seconds / 1000);
            std::cout << "FPS: " << fps << std::endl;
        }
        else
        {
            licznik_czas++;
        }
#endif
#endif

    }

    close_app = true;

    camera.release();

    shm_lidar_data.close();
    shm_lane_points.close();
    shm_usb_to_send.close();
    shm_watchdog.close();

    return 0;
}
