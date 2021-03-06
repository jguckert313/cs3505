//Bouncer: takes any pictures and creates an animation of a ball bouncing
//   this code uses the ffmpeg library
//Josh Bell and Jake Guckert

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <math.h>
#include <stdio.h>
#include "string.h"

AVFrame * convert(AVFrame * in, int format);

//Draw the circle by column within a circle boundary
void drawCircle(AVFrame * f, int cx, int cy, int rad)
{
  int row, col;
  int scale;
  for (col = cx-rad; col <= cx+rad; col++) {
    for(row = cy-rad; row <= cy+rad; row++)
      {	
	int dy = row-cy;
	int dx = col-cx;
	
	if(rad >= sqrt(dx*dx+dy*dy)){
	  //used to scale color for shading
	  uint8_t * pix = f->data[0] + (row * f->linesize[0]) + col*3;
	  *(pix++) = 255+scale; //(uint8_t) scale * r;
	  *(pix++) = 0+scale; //(uint8_t) scale * g;
	  *(pix++) = 0+scale; //(uint8_t) scale * b;
	}	
      } 
      scale+=1;
  }
  }

/**
 * Writes a frame to a file
 */
void writeFrame(AVFrame * in, int number) //number = sequence number
{
  FILE *pFile;
  char  filename[30];
  AVCodec *codec;
  AVCodecContext *c= NULL;
  int i, ret, x, y, got_output;
  AVFrame *frame;
  AVPacket pkt;

  sprintf(filename, "frame%d.utah", number);//print to a the array
  pFile=fopen(filename, "wb");
  if(pFile==NULL)
    return;

  printf("Encode video file %s\n", filename);

  av_init_packet(&pkt);
  pkt.data = NULL;    // packet data will be allocated by the encoder
  pkt.size = 0;

  /* find the mpeg1 video encoder */
  codec = avcodec_find_encoder(AV_CODEC_ID_UTAH);
  if (!codec) {
    fprintf(stderr, "Codec not found\n");
    exit(1);
  }
  
  c = avcodec_alloc_context3(codec);
  if (!c) {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }
  
  c->width = in->width;
  c->height = in->height;
  c->pix_fmt = codec->pix_fmts[0];//????????


  /* open it */
  if (avcodec_open2(c, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }

  frame = avcodec_alloc_frame();

  frame = convert(in, c->pix_fmt);
  

  /* encode the image */
  ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
  if (ret < 0) {
    fprintf(stderr, "Error encoding frame\n");
    exit(1);
  }

  if (got_output) {
    printf("Write frame %3d (size=%5d)\n", i, pkt.size);
    fwrite(pkt.data, 1, pkt.size, pFile);
    av_free_packet(&pkt);
  }
  
  fclose(pFile);
  avcodec_close(c);
  av_free(c);
}


/*
 * Converts frame to the file type specified
 */
AVFrame * convert(AVFrame * in, int format)
{
  int                    numBytes;
  uint8_t                *buffer = NULL;
  struct SwsContext      *sws_ctx = NULL;
  AVFrame                *pFrameRGB = NULL;

  // Determine required buffer size and allocate buffer
  numBytes=avpicture_get_size(format, in->width,
			      in->height);
  buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

  sws_ctx =
    sws_getContext
    (
     in->width,
     in->height,
     in->format,
     in->width,
     in->height,
     format,
     SWS_BILINEAR,
     NULL,
     NULL,
     NULL
     );

  // Allocate an AVFrame structure
  pFrameRGB=avcodec_alloc_frame();
  if(pFrameRGB==NULL)
    return NULL;


  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
  // of AVPicture
  avpicture_fill((AVPicture *)pFrameRGB, buffer, format,
		 in->width, in->height);

  
  // Convert the image from its native format to RGB
  sws_scale
    (
     sws_ctx,            //maybe not this int.
     (uint8_t const * const *)in->data, // the source of the pixels
     in->linesize, // how big the lines are on that source
     0,
     in->height, //destination height
     pFrameRGB->data,
     pFrameRGB->linesize
     );

	
  pFrameRGB->width = in->width;
  pFrameRGB->height = in->height;
  pFrameRGB->format = format;

  return pFrameRGB;
}


/*
 * take an image decodes to raw data
 */
AVFrame * loadFrame(char* filename)
{
  AVFormatContext *pFormatCtx = NULL;
  int             i, videoStream;
  AVCodecContext  *pCodecCtx = NULL;
  AVCodec         *pCodec = NULL;
  AVFrame         *pFrame = NULL; 
  AVFrame         *pFrameRGB = NULL;
  AVPacket        packet;
  int             frameFinished;
  AVDictionary    *optionsDict = NULL;
 
  // Register all formats and codecs
  av_register_all();
  
  // Open video file
  if(avformat_open_input(&pFormatCtx, filename, NULL, NULL)!=0)
    return NULL; // Couldn't open file
  
  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return NULL; // Couldn't find stream information
  
  // Dump information about file onto standard error
  av_dump_format(pFormatCtx, 0, filename, 0);
  
  // Find the first video stream
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return NULL; // Didn't find a video stream
  
  // Get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return NULL; // Codec not found
  }
  // Open codec
  if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
    return NULL; // Could not open codec
  
  // Allocate video frame
  pFrame=avcodec_alloc_frame();

  // Read frames and save first five frames to disk
  int done=0;
  while(av_read_frame(pFormatCtx, &packet)>=0 && !done) {
    // Is this a packet from the video stream?
    if(packet.stream_index==videoStream) {
      // Decode video frame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, 
			    &packet);

      // Did we get a video frame?
      if(frameFinished)
	pFrameRGB = convert(pFrame,AV_PIX_FMT_RGB24);
	  
    }
    done=1; //??????? as soon as get any frame
  }

  return pFrameRGB;
}

int main(int argc, char *argv[]) {
  
  if(argc < 2) {
    printf("Please provide a movie file\n");
    return -1;
  }
  char * temp = argv[1];
  char * tokens = strchr(temp, '.');

  if(strcmp(tokens, ".jpg") != 0) {
    printf("Please provide a jpg file\n");
    return -1;
  }

  AVFrame * orig = loadFrame(argv[1]);
  AVFrame * copy = convert(orig, AV_PIX_FMT_RGB24);
  AVFrame * workingFrame;
  int circleY = copy->height/2;
  int circleX = copy->width/2;
  int rad = copy->width/8;
  int direction = 0;
  int velocity = 8;
  int frameHeight = copy->height;


  int i;
  for(i=0; i < 300 ; i++)
    {
      //allocate space for a new copy to draw on
      workingFrame = avcodec_alloc_frame();
      workingFrame = convert(copy, AV_PIX_FMT_RGB24);

      //draw on working frame
      drawCircle(workingFrame, circleX, circleY, rad);
   
      // check the location of the ball and change direction if going off frame
      if(direction == 0)
        circleY += velocity;
      if(direction == 1)
        circleY -= velocity;
      if(circleY+rad+10 > frameHeight)
        direction = 1;
      if(circleY-rad-10 < 0)
        direction = 0;
      
      //drawcircle on copy
      writeFrame(workingFrame, i);

      av_free(workingFrame);
    }
  //free memory maybe
  
  return 0;
}
