#include "encodeandsend.h"
#include <stdlib.h>
#include <stdio.h>

EncodeAndSend::EncodeAndSend(char* url_in,char* IpAddress,int port,int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),QObject *parent) : QObject(parent)
{
    this->url_in=url_in;
    this->ptargetIp=new QHostAddress(IpAddress);
    this->targetPort=port;
    this->write_packet=write_packet;
}

int EncodeAndSend::InitAll()
{
    int ret;
    int width=640;
    int height=480;

    io_buffer=(uint8_t *)av_malloc(BUF_SIZE);

    pUdpSocket = new QUdpSocket();

    //registe all
    av_register_all();
    avformat_network_init();

    fd = v4l2_open(url_in, O_RDWR, 0);
    if (fd < 0)
    {
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    xioctl(fd, VIDIOC_S_FMT, &fmt);
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG)
    {
        printf("Libv4l didn't accept MJPEG format. Can't proceed.\n");
        exit(EXIT_FAILURE);
    }
    if ((fmt.fmt.pix.width != width) || (fmt.fmt.pix.height != height))
      printf("Warning: driver is sending image at %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);

    CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);

    buffers = (buffer*)calloc(req.count, sizeof(*buffers));
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        xioctl(fd, VIDIOC_QUERYBUF, &buf);

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = v4l2_mmap(NULL, buf.length,
        PROT_READ | PROT_WRITE, MAP_SHARED,
        fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
        {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < n_buffers; ++i)
    {
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        xioctl(fd, VIDIOC_QBUF, &buf);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    xioctl(fd, VIDIOC_STREAMON, &type);

    //init output format context
    ret=avformat_alloc_output_context2(&pOutputFormatCtx,NULL,"mpegts",NULL);
    if(ret<0)
    {
        qDebug()<<"could not allocate output formatctx context";
        return -4;
    }

    pIOOutCtx=avio_alloc_context(io_buffer,BUF_SIZE,1,NULL,NULL,write_packet,NULL);
    if(!pIOOutCtx)
    {
        qDebug()<<"could not allocate input io context";
        return -5;
    }
    pOutputFormatCtx->pb=pIOOutCtx;
    pOutputFormatCtx->flags=AVFMT_FLAG_CUSTOM_IO;

    //init codec context
    pCodec = avcodec_find_encoder_by_name(codec_name);
    if (!pCodec)
    {
        qDebug()<<"Codec not found";
        return -6;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx)
    {
        qDebug()<<"Could not allocate video codec context";
        return -7;
    }
    pCodecCtx->bit_rate = 1000000;
    pCodecCtx->width = width;
    pCodecCtx->height = height;
    pCodecCtx->time_base.num=1;
    pCodecCtx->time_base.den=10;
    pCodecCtx->framerate.num=10;
    pCodecCtx->framerate.den=1;
    pCodecCtx->gop_size = 10;
    pCodecCtx->max_b_frames = 0;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        qDebug()<<"Could not open codec";
        return -8;
    }

    //init decode context
    pDec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
    if (!pDec)
    {
        qDebug()<<"Codec not found";
        return -6;
    }

    pDecCtx = avcodec_alloc_context3(pDec);
    if (!pDecCtx)
    {
        qDebug()<<"Could not allocate video codec context";
        return -7;
    }
    pDecCtx->bit_rate = 1000000;
    pDecCtx->width = width;
    pDecCtx->height = height;
    pDecCtx->time_base.num=1;
    pDecCtx->time_base.den=10;
    pDecCtx->framerate.num=10;
    pDecCtx->framerate.den=1;
    pDecCtx->gop_size = 10;
    pDecCtx->max_b_frames = 0;
    pDecCtx->pix_fmt = AV_PIX_FMT_YUVJ422P;

    if (avcodec_open2(pDecCtx, pDec, NULL) < 0)
    {
        qDebug()<<"Could not open codec";
        return -8;
    }


    //init output stream
    pOutputStream=avformat_new_stream(pOutputFormatCtx,pCodec);
    if(!pOutputStream)
    {
        qDebug()<<"could not allocate output stream";
        return -9;
    }

    ret=avcodec_copy_context(pOutputStream->codec,pCodecCtx);
    if(ret<0)
    {
        qDebug()<<"could not set output stream codec context";
        return -10;
    }

    //init packet
    pInputPkt=av_packet_alloc();
    if(!pInputPkt)
    {
        qDebug()<<"could not allocate input packet";
        return -11;
    }
    pInputPkt->data=(uint8_t*)av_malloc(1000000);

    pOutputPkt=av_packet_alloc();
    if(!pOutputPkt)
    {
        qDebug()<<"could not allocate output packet";
        return -12;
    }

    //init frame
    pFrame = av_frame_alloc();
    if (!pFrame)
    {
        qDebug()<<"Could not allocate video frame";
        return -13;
    }
    pFrame->format = pCodecCtx->pix_fmt;
    pFrame->width  = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;

    pFrameYUV=av_frame_alloc();
    if (!pFrameYUV)
    {
        qDebug()<<"Could not allocate video frame";
        return -14;
    }
    pFrameYUV->format = AV_PIX_FMT_YUVJ422P;
    pFrameYUV->width  = pCodecCtx->width;
    pFrameYUV->height = pCodecCtx->height;

    ret=av_frame_get_buffer(pFrame,32);
    if (ret < 0)
    {
        qDebug()<<"Could not allocate raw picture buffer";
        return -15;
    }

    ret=av_frame_get_buffer(pFrameYUV,32);
    if (ret < 0)
    {
        qDebug()<<"Could not allocate raw picture buffer";
        return -16;
    }

    //init pix format switch
    rawToYUV420P=sws_getContext(pCodecCtx->width,pCodecCtx->height,AV_PIX_FMT_YUVJ422P,
                                pCodecCtx->width,pCodecCtx->height,AV_PIX_FMT_YUV420P,SWS_BILINEAR,NULL,NULL,NULL);
    return 0;
}

int EncodeAndSend::WriteHeader()
{
    int ret;
    ret=avformat_write_header(pOutputFormatCtx,NULL);
    if(ret<0)
    {
        qDebug()<<"could not write file header";
        return -1;
    }
    return 0;
}

int EncodeAndSend::ReadFrame()
{

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_DQBUF, &buf);

    memcpy(pInputPkt->data,buffers[buf.index].start,buf.bytesused);
    pInputPkt->size=buf.bytesused;

    xioctl(fd, VIDIOC_QBUF, &buf);
    return 0;
}

int EncodeAndSend::EncodeSend()
{
    int got_pic;
    avcodec_decode_video2(pDecCtx,pFrameYUV,&got_pic,pInputPkt);
    if(got_pic==0)
    {
        qDebug()<<"encode faile";
        exit(1);
    }
    sws_scale(rawToYUV420P,(const uchar* const*)pFrameYUV->data,pFrameYUV->linesize,0,
              pCodecCtx->height,pFrame->data,pFrame->linesize);

    int ret;
    ret=avcodec_send_frame(pCodecCtx,pFrame);
    if(ret<0)
    {
        qDebug()<<"could not send a frame to encoder";
        return -1;
    }

    while(ret>=0)
    {
        ret=avcodec_receive_packet(pCodecCtx,pOutputPkt);
        if(ret==AVERROR(EAGAIN) || ret==AVERROR_EOF)
            return 0;
        else if(ret<0)
        {
            qDebug()<<"error during encode";
            return -2;
        }

        ++frameNum;
        pOutputPkt->stream_index=0;
        pOutputPkt->duration=0;
        pOutputPkt->pts=frameNum;
        pOutputPkt->dts=frameNum;
        av_write_frame(pOutputFormatCtx,pOutputPkt);
        av_packet_unref(pOutputPkt);
        qDebug()<<frameNum;

    }
}

void EncodeAndSend::FreeAll()
{
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);
    for (int i = 0; i < n_buffers; ++i) {
    v4l2_munmap(buffers[i].start, buffers[i].length);
    }
    v4l2_close(fd);

    avcodec_free_context(&pCodecCtx);
    avcodec_free_context(&pDecCtx);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameYUV);
    av_packet_free(&pInputPkt);
    av_packet_free(&pOutputPkt);
}


