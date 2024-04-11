#include <iostream>
#include <chrono>
#include <thread>
#include <opencv2/video.hpp>
#include <opencv2/opencv.hpp>
#include <string>

#include "utils/logger.h"
#include "utils/timer.h"
#include "utils/udp_operation.h"

#define VAILD_LEN 1024

uint8_t check_sum(uint8_t *data, size_t len)
{
    uint8_t sum = 0;
    for (int i = 0; i < len; ++i)
    {
        sum = (sum + data[i]) & 0xff;
    }
    return sum;
}

void send_img(cv::Mat img, UDPOperation *server, int frame_idx, int win_idx, std::vector<int> win_point)
{
    uint8_t *buffer = new uint8_t[1041];
    buffer[0] = 0x81;
    buffer[1] = 0x05;
    buffer[2] = frame_idx;
    buffer[3] = win_idx;
    buffer[8] = (win_point[0] >> 8) & 0xff;
    buffer[9] = (win_point[0]) & 0xff;
    buffer[10] = (win_point[1] >> 8) & 0xff;
    buffer[11] = (win_point[1]) & 0xff;

    for (int i = 0; i < 256; i++)
    {
        // pack no
        buffer[6] = 0;
        buffer[7] = static_cast<uint8_t>(i);
        memcpy(buffer + 16, img.data + i * 1024, 1024);
        buffer[1040] = check_sum(buffer, 1040);
        server->send_buffer(reinterpret_cast<char *>(buffer), 1041);
    }
    delete[] buffer;
}

bool read_video(std::string video_file, std::vector<cv::Mat> *frame_set)
{
    cv::VideoCapture video(video_file, cv::CAP_ANY | cv::CAP_FFMPEG);
    if (!video.isOpened())
    {
        std::cout << "无法打开视频文件" << std::endl;
        return false;
    }
    cv::Mat frame;
    while (true)
    {
        // 读取视频帧
        bool ret = video.read(frame);
        if (!ret)
            break;

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        frame_set->push_back(gray.clone());
    }

    // 关闭视频文件
    video.release();
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: send_win <remote addres> <remote port> <img file>  \n"
                  << "\t<remote address>     \t Send to address\n"
                  << "\t<remote port>     \tSend to port\n"
                  << "\t<video1 file>    \t send video1 file\n"
                  << "\t<video2 file>    \t send video2 file\n"
                  << "Examples:\n"
                  << "\t ./send_win 192.168.1.111 8189  ./data/1_8bit_fps10.mp4  ./data/2_8bit_fps10.mp4\n"
                  << std::endl;
        return -1;
    }

    std::string address = *(argv + 1);
    int port = std::atoi(*(argv + 2));
    std::string video1_file = *(argv + 3);
    std::string video2_file = *(argv + 4);

    UDPOperation server(address.c_str(), port, "eth0");
    server.create_server();

    std::vector<cv::Mat> frame_set1;
    std::vector<cv::Mat> frame_set2;

    if (!read_video(video1_file, &frame_set1))
        throw std::runtime_error("无法打开视频文件");
    if (!read_video(video2_file, &frame_set2))
        throw std::runtime_error("无法打开视频文件");
    std::chrono::milliseconds interval(100);

    int frame1 = 0;
    int frame2 = 0;

    int direct1 = 0;
    int direct2 = 0;

    std::vector<int> win0_point{0, 0};
    std::vector<int> win1_point{15360 / 2, 0};
    MLOG_INFO("Start send");
    int cnt=0;
    while (true)
    {

        if (frame1 >= frame_set1.size() - 1)
        {
            direct1 = 1;
        }
        else if (frame1 <= 0)
        {
            /* code */
            direct1 = 0;
        }

        if (frame2 >= frame_set2.size() - 1)
        {
            direct2 = 1;
        }
        else if (frame2 <= 0)
        {
            /* code */
            direct2 = 0;
        }

        if (direct1)
        {
            frame1--;
        }
        else
        {
            frame1++;
        }
        if (direct2)
        {
            frame2--;
        }
        else
        {
            frame2++;
        }

        send_img(frame_set1[frame1], &server, cnt % 256, 0, win0_point);
        send_img(frame_set2[frame2], &server, cnt % 256, 1, win1_point);
        MLOG_INFO("win1 frame:%d,win2 frame:%d", frame1, frame2);

        win0_point[0] += 10;
        win0_point[1] += 24;
        if (win0_point[0] >= 15360 / 2 - 640 || win0_point[1] >= 17024 - 640)
        {
            win0_point[0] = 0;
            win0_point[1] = 0;
        }
        win1_point[0] += 10;
        win1_point[1] += 24;
        if (win1_point[0] >= 15360 - 640 || win1_point[1] >= 17024 - 640)
        {
            win1_point[0] = 15360 / 2;
            win1_point[1] = 0;
        }
        cnt++;
        std::this_thread::sleep_for(interval);
    }
    server.destory();
}
