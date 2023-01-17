#include <obs-module.h>

#include "client.h"

#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)

extern "C" {

static const char *output_getname(void *)
{
	return "imageflux_output";
}

static void *output_create(obs_data_t *, obs_output_t *output)
{
	auto data = new ImageFluxOutput();
	data->output = output;
	return (void *)data;
}

static void output_destroy(void *data)
{
	delete ((ImageFluxOutput *)data);
}

static bool output_start(void *data)
{
	return ((ImageFluxOutput *)data)->start();
}

static void output_stop(void *data, uint64_t)
{
	((ImageFluxOutput *)data)->stop();
}

static void output_video(void *data, struct video_data *frame)
{
	((ImageFluxOutput *)data)->video(frame);
}

static void output_audio(void *data, struct audio_data *frame)
{
	((ImageFluxOutput *)data)->audio(frame);
}

static uint64_t output_total_bytes(void *data)
{
	return ((ImageFluxOutput *)data)->totalBytes();
}

static int output_dropped_frames(void *data)
{
	return ((ImageFluxOutput *)data)->droppedFrames();
}

static float output_congestion(void *data)
{
	return ((ImageFluxOutput *)data)->congestion();
}

struct obs_output_info imageflux_output_info = {
	.id = "imageflux_output",
	.flags = OBS_OUTPUT_AV,
	.get_name = output_getname,
	.create = output_create,
	.destroy = output_destroy,
	.start = output_start,
	.stop = output_stop,
	.raw_video = output_video,
	.raw_audio = output_audio,
	.get_total_bytes = output_total_bytes,
	.get_dropped_frames = output_dropped_frames,
	.get_congestion = output_congestion,
};

}
