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

// FFmpeg
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

std::string streamAddr = "127.0.0.1";
int streamPort = 8080;
int resizeWidth = -1;
int resizeHeight = -1;

int byteToInt(char buf[])
{
    return ((buf[3] & 0xff) << 24) | ((buf[2] & 0xff) << 16) | ((buf[1] & 0xff) << 8) | (buf[0] & 0xff);
}

AVCodecContext *decoder_initialize(int type)
{
    AVCodecContext *c = NULL;
    switch (type)
    {
    case 0:
        return c;
    case 1:
        printf("format is H.264\n");
        const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec)
        {
            printf("error: avcodec_find_decoder\n");
            return c;
        }

        c = avcodec_alloc_context3(codec);
        if (!c)
        {
            printf("error: avcodec_aclloc_context3\n");
            return c;
        }
        // codec open
        if (avcodec_open2(c, codec, NULL) < 0)
        {
            printf("error: avcodec_open2\n");
            return c;
        }
        return c;
    }
    return NULL;
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
    if (boost::optional<std::string> val = pt.get_optional<std::string>("resize"))
    {
        std::cout << val.get() << std::endl;
        try
        {
            std::vector<std::string> split;
            boost::algorithm::split(split, val.get(), boost::is_any_of("x"));
            if (split.size() == 2)
            {
                int w = std::stoi(split[0]);
                int h = std::stoi(split[1]);
                if (w > 0 && h > 0)
                {
                    resizeWidth = w;
                    resizeHeight = h;
                }
            }
        }
        catch (std::exception const &e)
        {
            std::cout << "read resize parameter error: " << e.what() << std::endl;
        }
    }
}

void writeIniFile(char *fileName)
{
    boost::property_tree::ptree pt;
    pt.put("streamAddr", streamAddr);
    pt.put("streamPort", streamPort);
    pt.put("resize", std::to_string(resizeWidth) + "x" + std::to_string(resizeHeight));
    boost::property_tree::write_ini(fileName, pt);
}

int main(int argc, char **argv)
{

    // read ini file and load
    char *iniFileName = "property.ini";
    readIniFile(iniFileName);

    // write current setting to ini file
    writeIniFile(iniFileName);

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

        AVFrame *frame;
        AVFrame *frame_bgr;
        AVCodecContext *c = NULL;
        AVPacket *pkt;

        pkt = av_packet_alloc();
        if (!pkt)
        {
            printf("error: av_packet_alloc()\n");
            return -1;
        }

        frame = av_frame_alloc();
        frame_bgr = av_frame_alloc();
        if (!frame || !frame_bgr)
        {
            printf("error: av_frame_alloc()\n");
            return -1;
        }
        AVPixelFormat convFormat = AV_PIX_FMT_BGR24;
        frame_bgr->format = convFormat;
        uint8_t *bgrBuffer = NULL;
        int bgrBufferSize = -1;
        SwsContext *swsContext = NULL;
        uint8_t *matBuffer = NULL;

        bool decFlag = true;

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
            if (!c)
            {
                c = decoder_initialize(type);
                if (!c)
                {
                    printf("error: decoder initialize error\n");
                    return -1;
                }
                printf("H.264 decoder initialize succeed codec_id=%d\n", c->codec_id);
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

            // decode by FFmpeg
            av_init_packet(pkt);
            av_packet_from_data(pkt, (uint8_t *)recvBuf, length);
            if (pkt->size != length)
            {
                printf("error: ffmpeg size=%d, recvLen=%d\n", pkt->size, length);
                break;
            }

            ret = avcodec_send_packet(c, pkt);
            if (ret < 0)
            {
                printf("warn: avcodec_send_packet %d\n", ret);
                continue;
            }

            while (ret >= 0)
            {
                ret = avcodec_receive_frame(c, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    break;
                }
                else if (ret < 0)
                {
                    printf("error: avcodec_receive_frame %d\n", ret);
                    break;
                }

                // decode succeed
                int width = frame->width;
                int height = frame->height;

                // initialize
                if (bgrBufferSize < 0)
                {
                    bgrBufferSize = av_image_get_buffer_size(convFormat, width, height, 1);
                    bgrBuffer = (uint8_t *)av_malloc(sizeof(uint8_t) * bgrBufferSize);
                    av_image_fill_arrays(
                        frame_bgr->data, frame_bgr->linesize,
                        bgrBuffer, convFormat,
                        width, height, 1);
                    swsContext = sws_getContext(
                        width, height, c->pix_fmt,
                        width, height, convFormat,
                        SWS_BICUBIC, NULL, NULL, NULL);
                    if (!swsContext)
                    {
                        printf("error: sws_getContext()\n");
                        return -1;
                    }
                    matBuffer = (uint8_t *)malloc(sizeof(uint8_t) * bgrBufferSize);
                    printf("%dx%d, matSize=%d\n", width, height, bgrBufferSize);
                }

                // convert color format
                ret = sws_scale(
                    swsContext, frame->data, frame->linesize,
                    0, height,
                    frame_bgr->data, frame_bgr->linesize);
                if (ret < 0)
                {
                    printf("error: sws_scale error %d\n", ret);
                    continue;
                }

                // copy to matBuffer
                av_image_copy_to_buffer(matBuffer, bgrBufferSize,
                                        frame_bgr->data, frame_bgr->linesize, convFormat,
                                        width, height, 1);

                // mat and show
                cv::Mat mat(height, width, CV_8UC3, matBuffer);
                // resize if need
                if (resizeWidth > 0 && resizeHeight > 0)
                {
                    cv::resize(mat, mat, cv::Size(), resizeWidth/(double)width, resizeHeight/(double)height, cv::INTER_LINEAR);
                }
                cv::imshow("window", mat);
                if (cv::waitKey(1) == 27)
                {
                    loopFlag = false;
                }
                mat.release();
                decFlag = true;
            }

            free(recvBuf);
        }

        printf("%d loop exit\n", loopCounter++);

        // fclose(f);

        if (c)
            avcodec_free_context(&c);
        if (frame)
            av_frame_free(&frame);
        if (frame_bgr)
            av_frame_free(&frame_bgr);
        if (swsContext)
            sws_freeContext(swsContext);

        // close socket
        socket.close();
    }

    printf("---- finish ----\n");

    return 0;
}
