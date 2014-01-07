#include <iostream>
using namespace std;

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libswscale/swscale.h"
}

#undef exit

static const enum PixelFormat	kRawPixelFormat = PIX_FMT_BGRA;
static const int				kTargetFrameRate = 24;

static AVFrame *allocPicture(enum PixelFormat fmt, int width, int height);


inline unsigned int byteSwapInt(unsigned int in)
{
	return (((in & 0xFF000000) >> 24)
		|	((in & 0x00FF0000) >>  8)
		|	((in & 0x0000FF00) <<  8)
		|	((in & 0x000000FF) << 24));
}

inline unsigned long byteSwapLong(unsigned long in)
{
	return (((in & 0xFF00000000000000) >> 56)
		|	((in & 0x00FF000000000000) >> 40)
		|	((in & 0x0000FF0000000000) >> 24)
		|	((in & 0x000000FF00000000) >>  8)
		|	((in & 0x00000000FF000000) <<  8)
		|	((in & 0x0000000000FF0000) << 24)
		|	((in & 0x000000000000FF00) << 40)
		|	((in & 0x00000000000000FF) << 56));
}


class Frame
{
public:
	static AVFrame*	convertedPicture;
	static struct SwsContext* imageConverter;
	static uint8_t*	outputBuffer;
	static int		outputBufferSize;


	Frame(int w, int h)
	:	flipped(false)
	,	height(h)
	,	picture(NULL)
	,	rawTime(-1L)
	,	width(w)
	{
		picture = allocPicture(kRawPixelFormat, width, height);
	}

	~Frame();

	static void	cleanupStatics();
	void		drawCursor(int x, int y);
	bool		read(FILE* rawFile, int fileVersion);
	AVFrame*	setPicture(AVFrame* newPicture, long rawTime = -1);
	void		verticalFlip();
	void		write(AVFormatContext* outputCtx, AVStream* stream, int frameNum);

	bool		flipped;
	int			height;
	AVFrame*	picture;
	long		rawTime;
	int			width;
};


AVFrame*			Frame::convertedPicture = NULL;
struct SwsContext*	Frame::imageConverter = NULL;
uint8_t*			Frame::outputBuffer = NULL;
int					Frame::outputBufferSize = 0;


Frame::~Frame()
{
	if (picture) {
		av_free(picture->data[0]);
		av_free(picture);
	}
}


void Frame::cleanupStatics()
{
	if (convertedPicture) {
		av_free(convertedPicture->data[0]);
		av_free(convertedPicture);
	}

	if (imageConverter) {
		sws_freeContext(imageConverter);
		imageConverter = NULL;
	}

	if (outputBuffer) {
		av_free(outputBuffer);
		outputBuffer = NULL;
	}
}


void Frame::drawCursor(int x, int y)
{
	static int hotX = 2;
	static int hotY = 14;

	//	Cursor is applied before the image is flipped so data appears in
	//	OpenGL order with the first row of data being the bottom row of the
	//	cursor.
	//
	static unsigned short int cursor[] = {
		0x0000,
		0x0180,
		0x0180,
		0x0300,
		0x2300,
		0x3600,
		0x3e00,
		0x3fc0,
		0x3f80,
		0x3f00,
		0x3e00,
		0x3c00,
		0x3800,
		0x3000,
		0x2000,
		0x0000
	};

	static unsigned short int outline[] = {
		0x01c0,
		0x0240,
		0x0240,
		0x6480,
		0x5480,
		0x4900,
		0x4100,
		0x4020,
		0x4040,
		0x4080,
		0x4100,
		0x4200,
		0x4400,
		0x4800,
		0x5000,
		0x6000
	};

	for (int cursorRow = 0; cursorRow < 16; ++cursorRow) {
		int imageRow = y + cursorRow - hotY;

		if ((imageRow >= 0) && (imageRow < height)) {
			unsigned short int mask = 0x8000;

			for (int cursorCol = 0; cursorCol < 16; ++cursorCol) {
				int imageCol = x + cursorCol - hotX;

				if ((imageCol >= 0) && (imageCol < width)) {
					if ((cursor[cursorRow] & mask) != 0) {
						//	Set the pixel black.
						//
						int pixelStart = (imageRow * width + imageCol) * 4;

						picture->data[0][pixelStart] = 0x00;
						picture->data[0][pixelStart+1] = 0x00;
						picture->data[0][pixelStart+2] = 0x00;
						picture->data[0][pixelStart+3] = 0xff;
					} else if ((outline[cursorRow] & mask) != 0) {
						//	Set the pixel white.
						//
						int pixelStart = (imageRow * width + imageCol) * 4;

						picture->data[0][pixelStart] = 0xff;
						picture->data[0][pixelStart+1] = 0xff;
						picture->data[0][pixelStart+2] = 0xff;
						picture->data[0][pixelStart+3] = 0xff;
					}
				}

				mask = mask >> 1;
			}
		}
	}
}


bool Frame::read(FILE* rawFile, int fileVersion)
{
	//	Each frame begins with an 8-byte time in nanoseconds.
	//	Note that the time is in big endian form, so we have to byte swap
	//	it.
	//
	if (fread(&rawTime, sizeof(rawTime), 1, rawFile) != 1) {
		return false;
	}

	rawTime = byteSwapLong(rawTime);

	//	If this is file version 2 or later, get the coordinates of the
	//	mouse pointer.
	//
	int	ptrX = -1;
	int	ptrY = -1;

	if (fileVersion >= 2) {
		fread(&ptrX, sizeof(int), 1, rawFile);
		fread(&ptrY, sizeof(int), 1, rawFile);

		ptrX = byteSwapInt(ptrX);
		ptrY = byteSwapInt(ptrY);
	}

	//	Read the pixels.
	//
	fread(picture->data[0], 1, width * height * 4, rawFile);

	//	If the pointer coords are not negative that means that the system
	//	cursor was being used (as opposed to a graphical one) which won't
	//	have been captured in the image. So we'll add our own cursor
	//	instead.
	//
	if ((ptrX >= 0) && (ptrY >= 0)) {
		drawCursor(ptrX, ptrY);
	}

	flipped = false;

	return true;
}


void Frame::verticalFlip()
{
	//	The image was read directly from OpenGL, where the first row of
	//	pixels is the bottom of the image (Y == 0). Video wants the top row
	//	to be first, so we have to flip the image.
	//
	for (int row = 0; row < height / 2; ++row) {
		int bottomOffset = row * width * 4;
		int topOffset = (height - row - 1) * width * 4;

		for (int col = 0; col < width * 4; ++col) {
			uint8_t	temp = picture->data[0][bottomOffset + col];
			picture->data[0][bottomOffset + col] = picture->data[0][topOffset + col];
			picture->data[0][topOffset + col] = temp;
		}
	}

	flipped = true;
}


void Frame::write(AVFormatContext* outputCtx, AVStream* stream, int frameNum)
{
	AVCodecContext*	codecCtx = stream->codec;
	AVFrame*		picToWrite = picture;

	//	If the raw image hasn't been flipped yet, do it now.
	//
	if (!flipped) {
		verticalFlip();
	}

	//	If the raw image format doesn't match that used by the video format
	//	then we will have to convert the image format.
	//
	if (codecCtx->pix_fmt != kRawPixelFormat) {
		//	Create an conversion context, if we don't already have one.
		//
		if (imageConverter == NULL) {
			imageConverter = sws_getContext(
								width,
								height,
								kRawPixelFormat,
								codecCtx->width,
								codecCtx->height,
								codecCtx->pix_fmt,
								SWS_BICUBIC,
								NULL,
								NULL,
								NULL
							);

			if (imageConverter == NULL) {
				cerr << "Error getting image conversion context." << endl;
				exit(1);
			}
		}

		//	Create the conversion buffer, if it doesn't already exist.
		//
		if (convertedPicture == NULL) {
			convertedPicture = allocPicture(codecCtx->pix_fmt, codecCtx->width, codecCtx->height);
		}

		//	Convert the raw image from the input format to the output
		//	format.
		//
		sws_scale(
			imageConverter,
			picture->data,
			picture->linesize,
			0,
			codecCtx->height,
			convertedPicture->data,
			convertedPicture->linesize
		);

		picToWrite = convertedPicture;
	}

	picToWrite->pts = frameNum;

	int ret = 0;

	if (outputCtx->oformat->flags & AVFMT_RAWPICTURE) {
		AVPacket packet;

		av_init_packet(&packet);
		packet.stream_index = stream->index;
		packet.size = sizeof(AVPicture);
		packet.data = (uint8_t *)picToWrite;
		packet.flags |= AV_PKT_FLAG_KEY;

		ret = av_interleaved_write_frame(outputCtx, &packet);
	} else {
		//	Create the output buffer, if we haven't already.
		//
		if (outputBuffer == NULL) {
			//	Make the output buffer large enough to hold an entire,
			//	uncompressed image. That should always be big enough.
			//
			outputBufferSize = codecCtx->width * codecCtx->height * 4;
			outputBuffer = (uint8_t*)av_malloc(outputBufferSize);
		}

		//	Encode the image.
		//
		//	The return value is the encoded size. A result of zero means
		//	that the image has been buffered, so there will be no packets
		//	to output from it yet.
		//
		int encodedSize = avcodec_encode_video(codecCtx, outputBuffer, outputBufferSize, picToWrite);

		if (encodedSize > 0) {
			AVPacket packet;

			av_init_packet(&packet);
			packet.stream_index = stream->index;
			packet.size = encodedSize;
			packet.data = outputBuffer;

			if (codecCtx->coded_frame->pts != AV_NOPTS_VALUE) {
			    packet.pts= av_rescale_q(codecCtx->coded_frame->pts, codecCtx->time_base, stream->time_base);
			}

			if(codecCtx->coded_frame->key_frame) {
			    packet.flags |= AV_PKT_FLAG_KEY;
			}

			ret = av_interleaved_write_frame(outputCtx, &packet);
		}
	}

	if (ret != 0) {
		cerr << "Error while writing video frame." << endl;
		exit(1);
	}
}


static AVFrame *allocPicture(enum PixelFormat fmt, int width, int height)
{
	AVFrame* picture = avcodec_alloc_frame();

	if (picture) {
		int			buffSize = avpicture_get_size(fmt, width, height);
		uint8_t*	buff = (uint8_t*)av_malloc(buffSize);

		if (buff) {
			avpicture_fill((AVPicture *)picture, buff, fmt, width, height);
			picture->quality = 1;
		} else {
			av_free(picture);
			picture = NULL;
		}
	}

	return picture;
}


static AVStream* createStream(AVFormatContext *outputCtx, enum CodecID codecId, FILE* rawInput, int rawFileVersion)
{
	AVStream* stream = av_new_stream(outputCtx, 0);

	if (!stream) {
		cerr << "Could not alloc stream." << endl;
		exit(1);
	}

	AVCodecContext* codecCtx = stream->codec;

    //	Get the requested codec.
	//
    AVCodec* codec = avcodec_find_encoder(codecId);
    if (!codec) {
        cerr << "No codec found for id " << codecId << endl;
        exit(1);
    }

	avcodec_get_context_defaults2(codecCtx, codec->type);
	codecCtx->codec_id = codecId;

	//	Get the resolution from the first two ints in the raw file.
	//	Note that they are in big endian form, so we need to swap
	//	their bytes before setting them into the context.
	//
	fread(&codecCtx->width, sizeof(int), 1, rawInput);
	fread(&codecCtx->height, sizeof(int), 1, rawInput);

	codecCtx->width = (int)byteSwapInt((unsigned int)codecCtx->width);
	codecCtx->height = (int)byteSwapInt((unsigned int)codecCtx->height);

	//	Let's go with really high quality (i.e. low compression).
	//
	codecCtx->bit_rate = 16000000;

	//	In theory, I can set 'den' to 1,000 and then simply divide the
	//	nanosecond timestamps in the raw file by 1,000,000 to get my frame
	//	times, but when I tried that some apps were convinced that the
	//	frame rate was 1,000 fps and others had trouble keeping it synched.
	//
	//	So in practical terms it seems that 'den' pretty much has to be
	//	your target FPS.
	//
	codecCtx->time_base.den = kTargetFrameRate;
	codecCtx->time_base.num = 1;

	//	Max # of frames between interframes.
	//
	codecCtx->gop_size = 12;

	//	Let's assume that the first supported pixel format listed by the
	//	codec is its favourite/best one.
	//
	if (!codec->pix_fmts || (codec->pix_fmts[0] == -1)) {
		cerr << "Codec '" << codec->name << "' does not support any pixel formats!" << endl;
		exit(1);
	}

	codecCtx->pix_fmt = codec->pix_fmts[0];

    if (codecCtx->codec_id == CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        codecCtx->max_b_frames = 2;
    }

    if (codecCtx->codec_id == CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
           This does not happen with normal video, it just happens here as
           the motion of the chroma plane does not match the luma plane. */
        codecCtx->mb_decision = 2;
    }

	//	Cargo cult programming. I'm not really sure what this does but
	//	someone said it was needed to keep streams separate in formats
	//	which required that. I only have one frame so I'm not sure that I
	//	even need it. I may add an audio stream at some point so better
	//	safe than sorry.
	//
	if (outputCtx->oformat->flags & AVFMT_GLOBALHEADER) {
		codecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	if (avcodec_open(codecCtx, codec) < 0) {
		cerr << "Error opening codec " << codec->name << endl;
		exit(1);
	}

	return stream;
}


static int writeVideo(AVFormatContext* outputCtx, AVStream* stream, FILE* rawFile, int rawFileVersion)
{
	AVCodecContext*	codecCtx = stream->codec;
	Frame*	curFrame = new Frame(codecCtx->width, codecCtx->height);
	Frame*	nextFrame = new Frame(codecCtx->width, codecCtx->height);
	int		outFrameNum = 0;

	if (curFrame->read(rawFile, rawFileVersion)) {
		long firstFrameRawTime = curFrame->rawTime;

		//	Write out the first input frame as output frame 0.
		//
		curFrame->write(outputCtx, stream, outFrameNum++);

		//	Given the frame rate and the raw time of the first frame,
		//	what raw time do we expect for the next output frame?
		//
		long rawTimePerFrame = 1000000000L / kTargetFrameRate;
		long lastOutputTime = -rawTimePerFrame;
		long expectedRawTime = firstFrameRawTime + rawTimePerFrame;

		while (nextFrame->read(rawFile, rawFileVersion)) {
			//	If curFrame's raw time is closer than nextFrame's to what
			//	we're expecting for this output frame, write out a copy of
			//	curFrame as the current output frame, increment the
			//	output frame counter, and calculate the new output frame's
			//	expected time.
			//
			//	We keep doing this until the expected time is closer to
			//	nextFrame than to curFrame.
			//
			while ((expectedRawTime - curFrame->rawTime)
						< (nextFrame->rawTime - expectedRawTime)) {
				curFrame->write(outputCtx, stream, outFrameNum++);
				lastOutputTime = expectedRawTime;
				expectedRawTime += rawTimePerFrame;

				//	Display a progress message every 100 frames.
				//
				if (outFrameNum % 100 == 0) {
					cout << outFrameNum << " frames" << endl;
				}
			}

			//	nextFrame is now closer than curFrame so swap them. The old
			//	curFrame will be recycled for use with the next nextFrame.
			//
			Frame* temp = curFrame;
			curFrame = nextFrame;
			nextFrame = temp;
		}

		//	The last frame of the input file is sitting in curFrame. So
		//	long as our expected time for the current output frame is less
		//	than curFrame's time, keep outputting copies of curFrame.
		//
		while (expectedRawTime < curFrame->rawTime) {
			curFrame->write(outputCtx, stream, outFrameNum++);
			lastOutputTime = expectedRawTime;
			expectedRawTime += rawTimePerFrame;
		}

		//	The expected time for the current output frame is either equal
		//	to or greater than curFrame's time. If it's closer to the
		//	current frame's expected time than it was to the last one,
		//	output a final copy of curFrame as the final frame of the
		//	video.
		//
		if ((expectedRawTime - curFrame->rawTime) < (curFrame->rawTime - lastOutputTime)) {
			curFrame->write(outputCtx, stream, outFrameNum++);
		}

		delete nextFrame;
	}

	delete curFrame;

	return outFrameNum;
}


int main(int argc, char **argv)
{
	//	Initialize the av libs and register all codecs and formats.
	//
	av_register_all();

	if (argc != 3) {
		cout << "usage: " << argv[0] << " raw_file output_file" << endl << endl;
		cout << "Converts a raw file generated by the FrameGrabber mod into a video file." << endl;
		cout << "The video format is selected based on the file extension, defaulting to" << endl;
		cout << "mpeg if no codec can be found for that format." << endl;
		return 1;
	}

	FILE* rawFile = fopen(argv[1], "r");

	if (!rawFile) {
		cerr << "Could not open '" << argv[1] << "' for read." << endl;
		return 1;
	}

	//	The first int in the file is the file version.
	//
	int	rawFileVersion = -1;
	fread(&rawFileVersion, sizeof(int), 1, rawFile);
	rawFileVersion = byteSwapInt(rawFileVersion);

	//	Version 1 of the raw file did not contain a version number, but
	//	instead started with the width of the image. We will assume that no
	//	videos were recorded with a width less than 320 so if the version
	//	number we read is >= 320 then it must be a version 1 file.
	//
	if (rawFileVersion >= 320) {
		rawFileVersion = 1;
		rewind(rawFile);
	}

	const char *outputFilename = argv[2];

	AVFormatContext*	outputCtx = avformat_alloc_context();
	AVOutputFormat*		outputFormat = av_guess_format(NULL, outputFilename, NULL);

	if (!outputFormat) {
		cerr << "Could not determine output format from file extension: defaulting to mpeg." << endl;
		outputFormat = av_guess_format("mpeg", outputFilename, NULL);

		if (!outputFormat) {
			cerr << "Error: mpeg not supported. It should ALWAYS be supported so check your ffmpeg install." << endl;
			return 1;
		}
	}

	outputCtx->oformat = outputFormat;
	strncpy(outputCtx->filename, outputFilename, sizeof(outputCtx->filename));

	if (outputFormat->video_codec != CODEC_ID_NONE) {
		//	Create the video output stream.
		//
		AVStream *videoStream = createStream(outputCtx, outputFormat->video_codec, rawFile, rawFileVersion);

		if (videoStream) {
			//	Let the user know what we're trying to do.
			//
			dump_format(outputCtx, 0, outputFilename, 1);

			//	Open the output file.
			//
			if (!(outputFormat->flags & AVFMT_NOFILE)) {
				if (url_fopen(&outputCtx->pb, outputFilename, URL_WRONLY) < 0) {
					cout << "Error opening '" << outputFilename << "' for output." << endl;
					return 1;
				}
			}

			//	Output the stream header.
			//
			av_write_header(outputCtx);

			//	Output the video frames.
			//
			writeVideo(outputCtx, videoStream, rawFile, rawFileVersion);

			//	Output the stream trailer.
			//
			av_write_trailer(outputCtx);

			//	Close the codec.
			//
			avcodec_close(videoStream->codec);
		}
	}

	//	Clean up.
	//
	for (int i = 0; i < outputCtx->nb_streams; ++i) {
		av_freep(&outputCtx->streams[i]->codec);
		av_freep(&outputCtx->streams[i]);
	}

	if (!(outputFormat->flags & AVFMT_NOFILE)) {
		url_fclose(outputCtx->pb);
	}

	av_free(outputCtx);

	Frame::cleanupStatics();

	return 0;
}

