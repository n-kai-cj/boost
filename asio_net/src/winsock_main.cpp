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

//#include <sys/socket.h>
//#include <arpa/inet.h>

// WINDOWS
#if _WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

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

AVCodecContext *decoder_initialize()
{
    AVCodecContext *c = NULL;
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

void closeWinSocket()
{
    // WINDOWS
    WSACleanup();
}

int connectWinSocket(char *streamAddr, int streamPort)
{

    // WINDOWS
    WSADATA data;
    WSAStartup(MAKEWORD(2, 0), &data);

    int mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // set socket option
    int sockBufSize = 100 * 1024 * 1024;
    setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (char *)sockBufSize, sizeof(int));
    setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, (char *)sockBufSize, sizeof(int));
    int sockTimeoutMsec = 500 * 1000;
    setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)sockTimeoutMsec, sizeof(int));
    setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)sockTimeoutMsec, sizeof(int));

    struct sockaddr_in addr;
    addr.sin_port = htons(streamPort);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(streamAddr);

    // try to connect...
    int ret = connect(mSocket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        printf("error not connected %s:%d\n", streamAddr, streamPort);
        Sleep(2 * 1000);
        closeWinSocket();
        return ret;
    }

    return mSocket;
}

int main(int argc, char **argv)
{

    char lenBuf[4];
    int loopCounter = 0;
    int ret;
    bool loopFlag = true;

    char *streamAddr = "127.0.0.1";
    int streamPort = 8080;

    while (loopFlag)
    {
        printf("try connect to %s:%d... ", streamAddr, streamPort);
        // make socket and try to connect
        int mSocket = connectWinSocket(streamAddr, streamPort);
        if (mSocket < 0)
        {
            continue;
        }
        printf("connection succeed\n");

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

        // FILE *f = fopen("test.264", "wb");

        // receive, decode, and display loop
        while (loopFlag)
        {
            if (!decFlag)
            {
                printf("dec failed\n");
            }
            decFlag = false;
            // initialize decoder
            if (!c)
            {
                c = decoder_initialize();
                if (!c)
                {
                    printf("error: decoder initialize error\n");
                    return -1;
                }
                printf("H.264 decoder initialize succeed codec_id=%d\n", c->codec_id);
            }

            // receive data length
            bufLen = recv(mSocket, lenBuf, 4, 0);
            if (bufLen < 4)
            {
                printf("error: recv len continue\n");
                break;
            }
            int length = ((lenBuf[3] & 0xff) << 24) | ((lenBuf[2] & 0xff) << 16) | ((lenBuf[1] & 0xff) << 8) | (lenBuf[0] & 0xff);
            if (length <= 0)
            {
                printf("error. length = %d\n", length);
                break;
            }

            // receive video data
            char *recvBuf = (char *)malloc(sizeof(char) * length);

            bufLen = 0;
            while (bufLen < length)
            {
                bufLen += recv(mSocket, recvBuf + bufLen, length - bufLen, 0);
            }
            if (bufLen != length)
            {
                printf("error: data length=%d, but recv len=%d\n", length, bufLen);
                free(recvBuf);
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
        closeWinSocket();
    }

    printf("---- finish ----\n");
    return 0;
}
