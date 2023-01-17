#include <obs-module.h>
#include <util/dstr.h>

struct imageflux_options {
	char *signaling_url;
	char *channel_id;
	bool multistream;

	bool audio;
	int audio_bit_rate;
	char *audio_codec_type;

	bool video;
	int video_bit_rate;
	char *video_codec_type;
};

static const char *encoder_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "ImageFlux Live Streaming (WebRTC)";
}

static void encoder_destroy(void *data)
{
	struct imageflux_options *encoder = data;
	bfree(encoder);
}

static void *encoder_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	UNUSED_PARAMETER(settings);
	UNUSED_PARAMETER(encoder);
	struct imageflux_options *data = bzalloc(sizeof(struct imageflux_options));
	return data;
}

static void encoder_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "video", true);
	obs_data_set_default_string(settings, "video_codec_type", "VP8");
	obs_data_set_default_int(settings, "video_bit_rate", 800);
	obs_data_set_default_bool(settings, "audio", true);
	obs_data_set_default_string(settings, "audio_codec_type", "OPUS");
	obs_data_set_default_int(settings, "audio_bit_rate", 64);
	obs_data_set_default_bool(settings, "multistream", false);
}

static bool setting_video_modified(obs_properties_t *ppts,
				obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	bool enabled = obs_data_get_bool(settings, "video");
	obs_property_set_enabled(obs_properties_get(ppts, "video_codec_type"), enabled);
	obs_property_set_enabled(obs_properties_get(ppts, "video_bit_rate"), enabled);
	return true;
}

static bool setting_audio_modified(obs_properties_t *ppts,
				obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	bool enabled = obs_data_get_bool(settings, "audio");
	obs_property_set_enabled(obs_properties_get(ppts, "audio_codec_type"), enabled);
	obs_property_set_enabled(obs_properties_get(ppts, "audio_bit_rate"), enabled);
	return true;
}

static obs_properties_t *encoder_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(ppts, "signaling_url", "Signaling URL", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "channel_id", "Channel ID", OBS_TEXT_DEFAULT);
	obs_properties_add_bool(ppts, "multistream", "Multistream");

	p = obs_properties_add_bool(ppts, "video", "Video");
	obs_property_set_modified_callback(p, setting_video_modified);

	p = obs_properties_add_list(ppts, "video_codec_type", "Video Codec", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, "H264", "H264");
	obs_property_list_add_string(p, "VP8", "VP8");
	obs_property_list_add_string(p, "VP9", "VP9");
	obs_property_list_add_string(p, "AV1", "AV1");

	obs_properties_add_int(ppts, "video_bit_rate", "Video Bitrate", 100, 3000, 100);

	p = obs_properties_add_bool(ppts, "audio", "Audio");
	obs_property_set_modified_callback(p, setting_audio_modified);

	p = obs_properties_add_list(ppts, "audio_codec_type", "Audio Codec", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, "OPUS", "OPUS");

	obs_properties_add_int(ppts, "audio_bit_rate", "Audio Bitrate", 32, 160, 32);

	return ppts;
}

// don't delete this function
// 削除禁止 これがないと設定画面に選択肢が出ない
static bool encoder_encode(void *data, struct encoder_frame *frame,
		       struct encoder_packet *packet, bool *received_packet)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(frame);
	UNUSED_PARAMETER(packet);
	UNUSED_PARAMETER(received_packet);
	return false;
}

struct obs_encoder_info imageflux_encoder_info = {
	.id = "imageflux_encoder",
	.type = OBS_ENCODER_VIDEO,
	.codec = "h264", // OBSはh264,h265しか配信用コーデック扱いしてくれない
	.get_name = encoder_name,
	.create = encoder_create,
	.destroy = encoder_destroy,
	.encode = encoder_encode, // これがないと設定画面に選択肢が出ない
	.get_defaults = encoder_defaults,
	.get_properties = encoder_properties,
};
