/*
 * Copyright (C) 2019 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/optional.hpp>

// OpenCV
#include <opencv2/opencv.hpp>

// Boost
#include <boost/asio.hpp>

// MFX
#include "intel_qsv_decoder_api.h"

std::string streamAddr = "127.0.0.1";
int streamPort = 5550;

int byteToInt(char buf[])
{
    return ((buf[3] & 0xff) << 24) | ((buf[2] & 0xff) << 16) | ((buf[1] & 0xff) << 8) | (buf[0] & 0xff);
}

void sleep(int sleepMsec)
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(sleepMsec));
}

void readIniFile(char *fileName)
{
    boost::property_tree::ptree pt;
    try
    {
        boost::property_tree::read_ini(fileName, pt);
    }
    catch (boost::property_tree::ptree_error &e)
    {
        return;
    }

    if (boost::optional<std::string> val = pt.get_optional<std::string>("streamAddr"))
    {
        std::cout << val.get() << std::endl;
        streamAddr = val.get();
    }
    if (boost::optional<int> val = pt.get_optional<int>("streamPort"))
    {
        std::cout << val.get() << std::endl;
        streamPort = val.get();
    }
}

void writeIniFile(char *fileName)
{
    boost::property_tree::ptree pt;
    pt.put("streamAddr", streamAddr);
    pt.put("streamPort", streamPort);
    boost::property_tree::write_ini(fileName, pt);
}

int main(int argc, char **argv)
{

    // read ini file and load
    char *iniFileName = "property.ini";
    readIniFile(iniFileName);

    // write current setting to ini file
    writeIniFile(iniFileName);

    // opencv window name
    const char* windowName = "window";
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);

    bool loopFlag = true;
    int ret;
    int loopCounter = 0;
    char typeBuf[4];
    char tsBuf[4];
    char lenBuf[4];

    // make boost socket
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket(io_service);
    boost::system::error_code error;

    while (loopFlag)
    {
        // connect
        printf("try connect to %s:%d... ", streamAddr, streamPort);
        socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(streamAddr), streamPort), error);
        if (error)
        {
            printf("error: boost connect failed %s\n", error.message().c_str());
            socket.close();
            sleep(2 * 1000);
            continue;
        }

        printf("connect succeed\n");

        // set socket option
        socket.set_option(boost::asio::socket_base::receive_buffer_size(100 * 1024 * 1024));
        socket.set_option(boost::asio::socket_base::send_buffer_size(100 * 1024 * 1024));

        IntelQsvH264Decoder* dec = NULL;
        int width, height;
        uint8_t *matBuffer = NULL;

        // receive, decode, and display loop
        while (loopFlag)
        {

            // receive video type
            int bufLen = boost::asio::read(socket, boost::asio::buffer(typeBuf, 4), boost::asio::transfer_exactly(4), error);
            if (bufLen < 4)
            {
                printf("error: recv type continue. bufLen=%d\n", bufLen);
                break;
            }
            if (error)
            {
                printf("error: boost read type failed %s\n", error.message().c_str());
                break;
            }
            int type = byteToInt(typeBuf);
            if (type != 1)
            {
                printf("something type error %d\n", type);
                break;
            }

            // initialize decoder
            if (dec == NULL)
            {
                if (type != 1)
                {
                    printf("error: codec type is not supported. %d\n", type);
                    break;
                }
                dec = new IntelQsvH264Decoder();
                if (dec->initialize() < 0)
                {
                    printf("error: decoder initialize error\n");
                    return -1;
                }
                width = dec->getWidth();
                height = dec->getHeight();
            printf("H.264 decoder initialize succeed\n");
            }

            // receive timestamp
            bufLen = boost::asio::read(socket, boost::asio::buffer(tsBuf, 4), error);
            if (bufLen < 4)
            {
                printf("error: recv ts. bufLen=%d\n", bufLen);
                break;
            }
            if (error)
            {
                printf("error: boost read timestamp failed %s\n", error.message().c_str());
                break;
            }
            int timestamp = byteToInt(tsBuf);
#if DEBUG
            printf("timestamp = %d\n", timestamp);
#endif

            // receive data length
            bufLen = boost::asio::read(socket, boost::asio::buffer(lenBuf, 4), boost::asio::transfer_exactly(4), error);
            if (bufLen < 4)
            {
                printf("error: recv length. bufLen=%d\n", bufLen);
                break;
            }
            if (error)
            {
                printf("error: boost read length failed %s\n", error.message().c_str());
                break;
            }
            int length = byteToInt(lenBuf);

            // receive video data
            char *recvBuf = (char *)malloc(sizeof(char) * length);
            bufLen = boost::asio::read(socket, boost::asio::buffer(recvBuf, length), boost::asio::transfer_exactly(length), error);
            if (bufLen != length)
            {
                printf("error: data length=%d, but recv len=%d\n", length, bufLen);
                break;
            }
            if (error)
            {
                printf("error: boost read data failed %s\n", error.message());
                break;
            }

            // parse ES data and ready to decode
            if (!dec->isInit())
            {
                if (dec->decodeHeader((uint8_t*)recvBuf, length) < 0)
                {
                    printf("warn: decodeHeader failed. length=%d\n", length);
                    free(recvBuf);
                    continue;
                }
            }
            // decode by Intel Qsv
            ret = dec->decode((uint8_t*)recvBuf, length);
            if (ret != 0)
            {
                printf("error: decode error. ret=%d\n", ret);
                free(recvBuf);
                continue;
            }
            if (width != dec->getWidth() || height != dec->getHeight())
            {
                printf("param changed %dx%d -> %dx%d\n", width, height, dec->getWidth(), dec->getHeight());
                width = dec->getWidth();
                height = dec->getHeight();
                if (matBuffer)
                {
                    free(matBuffer);
                }
                matBuffer = (uint8_t*)malloc(sizeof(uint8_t) * width * height * 3);
            }
            ret = dec->getFrame(matBuffer, 1);

            while (ret > 0)
            {
                // mat and show
                cv::Mat mat(height, width, CV_8UC3, matBuffer);
                cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
                cv::imshow(windowName, mat);
                if (cv::waitKey(1) == 27)
                {
                    loopFlag = false;
                }
                mat.release();
                ret = dec->getFrame(matBuffer, 1);
            }

            free(recvBuf);
        }

        printf("%d loop exit\n", loopCounter++);

        if (dec != NULL)
        {
            dec->uninitialize();
        }
        if (matBuffer != NULL)
        {
            free(matBuffer);
        }

        // close socket
        socket.close();
    }

    printf("---- finish ----\n");

    return 0;
}
