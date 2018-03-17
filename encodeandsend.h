#ifndef ENCODEANDSEND_H
#define ENCODEANDSEND_H

#define  BUF_SIZE  4096
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#include <QObject>
#include <QtNetwork>
#include <QDebug>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <libv4l1.h>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}


class EncodeAndSend : public QObject
{
    Q_OBJECT
public:
    EncodeAndSend(char* url_in,char* IpAddress,int port,int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),QObject *parent = 0);
    int InitAll();
    int WriteHeader();
    int ReadFrame();
    int EncodeSend();
    void FreeAll();

    QUdpSocket *pUdpSocket;
    QHostAddress *ptargetIp;
    int targetPort;
private:

    AVFormatContext *pInputFormatCtx;
    AVFormatContext *pOutputFormatCtx;
    AVInputFormat   *pInputFormat;
    AVOutputFormat  *pOutputFormat;
    AVIOContext     *pIOOutCtx;
    AVStream        *pOutputStream;
    AVCodec         *pCodec;
    AVCodec         *pDec;
    AVCodecContext  *pCodecCtx;
    AVCodecContext  *pDecCtx;
    AVFrame         *pFrame;
    AVFrame         *pFrameYUV;
    AVPacket        *pInputPkt;
    AVPacket        *pOutputPkt;
    struct SwsContext *rawToYUV420P;
    int frameNum=0;

    char* url_in;
    uint8_t* io_buffer;
    char* codec_name="h264_omx";
    FILE* fp;

    int (*write_packet)(void *opaque, uint8_t *buf, int buf_size);

    //////////////////////////////////////////////////////////
    struct buffer {
      void   *start;
      size_t length;
    };

    static void xioctl(int fh, int request, void *arg)
    {
      int r;

      do {
        r = v4l2_ioctl(fh, request, arg);
      } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

      if (r == -1) {
        fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
      }
    }

      struct v4l2_format              fmt;
      struct v4l2_buffer              buf;
      struct v4l2_requestbuffers      req;
      enum v4l2_buf_type              type;
      fd_set                          fds;
      struct timeval                  tv;
      int                             fd = -1;
      unsigned int                    n_buffers;
      struct buffer                   *buffers;
};

#endif // ENCODEANDSEND_H
