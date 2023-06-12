#include "client.h"

// WebRTC
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
#include <api/rtc_event_log/rtc_event_log_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <media/engine/webrtc_media_engine.h>
#include <modules/audio_device/include/audio_device.h>
#include <modules/audio_device/include/audio_device_factory.h>
#include <modules/audio_processing/include/audio_processing.h>
#include <modules/video_capture/video_capture.h>
#include <modules/video_capture/video_capture_factory.h>
#include <pc/video_track_source_proxy.h>
#include <rtc_base/logging.h>
#include <rtc_base/ssl_adapter.h>
#include <api/video/i420_buffer.h>

// Sora
#include <sora/audio_device_module.h>
#include <sora/camera_device_capturer.h>
#include <sora/java_context.h>
#include <sora/rtc_stats.h>
#include <sora/sora_peer_connection_factory.h>
#include <sora/sora_video_decoder_factory.h>
#include <sora/sora_video_encoder_factory.h>
#include <sora/mac/mac_video_factory.h>

#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR, format, ##__VA_ARGS__)

SoraClient::SoraClient() {}

SoraClient::~SoraClient()
{
	info("SoraClient::dtor() ***");
	video_source_ = nullptr;
	audio_source_ = nullptr;
	adm_ = nullptr;

	audio_track_ = nullptr;
	video_track_ = nullptr;
	connection_context_ = nullptr;
	factory_ = nullptr;

	if (ioc_ != nullptr) {
		ioc_->stop();
	}
	signaling_.reset();
	ioc_.reset();
	if (io_thread_) {
		io_thread_->Stop();
		io_thread_.reset();
	}
	if (network_thread_) {
		network_thread_->Stop();
		network_thread_.reset();
	}
	if (worker_thread_) {
		worker_thread_->Stop();
		worker_thread_.reset();
	}
	if (signaling_thread_) {
		signaling_thread_->Stop();
		signaling_thread_.reset();
	}
}

OBSAudioSource::OBSAudioSource(){
	size_t pending_len = 2 * 2 * 640; // num_channel(stereo) * sample_size(short) * sample_rate(48khz) / chunk(100Hz)
	pending = (uint8_t *)malloc(pending_len);
	pending_remainder = 0;

	options_.echo_cancellation.emplace(false);
	options_.auto_gain_control.emplace(false);
	options_.noise_suppression.emplace(false);
	options_.highpass_filter.emplace(false);
	// options_.stereo_swapping.emplace(false);
	// options_.audio_jitter_buffer_fast_accelerate.emplace(false);
	// options_.combined_audio_video_bwe.emplace(false);
	// options_.audio_network_adaptor.emplace(false);
	// options_.init_recording_on_send.emplace(false);

	sink_ = nullptr;
}

OBSAudioSource::~OBSAudioSource(){
	free(pending);
}

void OBSAudioSource::OnAudioData(audio_data *frame)
{
	webrtc::AudioTrackSinkInterface *sink = this->sink_;
	if (!sink) {
		return;
	}

	uint8_t *data = frame->data[0];
	size_t num_channels = 2;
	uint32_t sample_rate = 48000;
	size_t chunk = (sample_rate / 100);
	size_t sample_size = 2;
	size_t i = 0;
	uint8_t *position;
	if (pending_remainder) {
		i = chunk - pending_remainder;
		memcpy(pending + pending_remainder * sample_size * num_channels,
		       data, i * sample_size * num_channels);
		sink->OnData(pending, 16, sample_rate, num_channels, chunk);
		pending_remainder = 0;
	}

	while (i + chunk < frame->frames) {
		position = data + i * sample_size * num_channels;
		sink->OnData(position, 16, sample_rate, num_channels, chunk);
		i += chunk;
	}

	if (i != frame->frames) {
		pending_remainder = (uint16_t)(frame->frames - i);
		memcpy(pending, data + i * sample_size * num_channels,
		       pending_remainder * sample_size * num_channels);
	}
}

void SoraClient::Connect()
{
	rtc::InitializeSSL();

	network_thread_ = rtc::Thread::CreateWithSocketServer();
	network_thread_->Start();
	worker_thread_ = rtc::Thread::Create();
	worker_thread_->Start();
	signaling_thread_ = rtc::Thread::Create();
	signaling_thread_->Start();

	webrtc::PeerConnectionFactoryDependencies dependencies;
	dependencies.network_thread = network_thread_.get();
	dependencies.worker_thread = worker_thread_.get();
	dependencies.signaling_thread = signaling_thread_.get();
	dependencies.task_queue_factory =
		webrtc::CreateDefaultTaskQueueFactory();
	dependencies.call_factory = webrtc::CreateCallFactory();
	dependencies.event_log_factory =
		absl::make_unique<webrtc::RtcEventLogFactory>(
			dependencies.task_queue_factory.get());

	// media_dependencies
	cricket::MediaEngineDependencies media_dependencies;
	media_dependencies.task_queue_factory = dependencies.task_queue_factory.get();
	adm_ = webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kDummyAudio,
		media_dependencies.task_queue_factory);

	media_dependencies.adm = adm_;

	media_dependencies.audio_encoder_factory =
		webrtc::CreateBuiltinAudioEncoderFactory();
	media_dependencies.audio_decoder_factory =
		webrtc::CreateBuiltinAudioDecoderFactory();

	{
#ifdef __APPLE__
		media_dependencies.video_encoder_factory =
			sora::CreateMacVideoEncoderFactory();
#else
		void *env = sora::GetJNIEnv();
		auto cuda_context = sora::CudaContext::Create();
		auto config = sora::GetDefaultVideoEncoderFactoryConfig(
			cuda_context, env);
		// config.use_simulcast_adapter = true;
		media_dependencies.video_encoder_factory =
			absl::make_unique<sora::SoraVideoEncoderFactory>(
				std::move(config));
#endif
	}
	media_dependencies.video_decoder_factory = nullptr;
	// {
	// 	auto config = sora::GetDefaultVideoDecoderFactoryConfig(
	// 		cuda_context, env);
	// 	media_dependencies.video_decoder_factory =
	// 		absl::make_unique<sora::SoraVideoDecoderFactory>(
	// 			std::move(config));
	// }

	media_dependencies.audio_mixer = nullptr;
	media_dependencies.audio_processing =
		webrtc::AudioProcessingBuilder().Create();

	dependencies.media_engine =
		cricket::CreateMediaEngine(std::move(media_dependencies));

	factory_ = sora::CreateModularPeerConnectionFactoryWithContext(
		std::move(dependencies), connection_context_);

	if (factory_ == nullptr) {
		RTC_LOG(LS_ERROR) << "Failed to create PeerConnectionFactory";
		// on_disconnect((int)sora::SoraSignalingErrorCode::INTERNAL_ERROR,
		// 				"Failed to create PeerConnectionFactory");
		return;
	}

	webrtc::PeerConnectionFactoryInterface::Options factory_options;
	factory_options.disable_encryption = false;
	factory_options.ssl_max_version = rtc::SSL_PROTOCOL_DTLS_12;
	factory_options.crypto_options.srtp.enable_gcm_crypto_suites = true;
	factory_->SetOptions(factory_options);

	{
		audio_source_ = rtc::make_ref_counted<OBSAudioSource>();
		std::string audio_track_id = rtc::CreateRandomString(16);
		audio_track_ = factory_->CreateAudioTrack(audio_track_id, audio_source_.get());
	}
	{
		sora::ScalableVideoTrackSourceConfig config = {};
		video_source_ = rtc::make_ref_counted<sora::ScalableVideoTrackSource>(config);
		std::string video_track_id = rtc::CreateRandomString(16);
		video_track_ = factory_->CreateVideoTrack(video_track_id, video_source_.get());
		video_track_->set_content_hint(webrtc::VideoTrackInterface::ContentHint::kFluid); // framerate
		// video_track_->set_content_hint(webrtc::VideoTrackInterface::ContentHint::kDetailed); // resolution
	}
	{
		obs_encoder_t * encoder = obs_output_get_video_encoder(output);
		auto settings = obs_encoder_get_settings(encoder);
		auto signaling_url = obs_data_get_string(settings, "signaling_url");
		auto channel_id = obs_data_get_string(settings, "channel_id");
		auto multistream = obs_data_get_bool(settings, "multistream");
		auto video = obs_data_get_bool(settings, "video");
		auto video_codec_type = obs_data_get_string(settings, "video_codec_type");
		auto video_bit_rate = obs_data_get_int(settings, "video_bit_rate");
		auto audio = obs_data_get_bool(settings, "audio");
		auto audio_codec_type = obs_data_get_string(settings, "audio_codec_type");
		auto audio_bit_rate = obs_data_get_int(settings, "audio_bit_rate");

		ioc_.reset(new boost::asio::io_context(1));
		sora::SoraSignalingConfig sconfig;
		sconfig.observer = shared_from_this();
		sconfig.pc_factory = factory_;
		sconfig.io_context = ioc_.get();
		sconfig.role = "sendonly"; // config_.role;
		sconfig.sora_client = "OBS29.1.2";
		sconfig.multistream = multistream;

		sconfig.signaling_urls.push_back(signaling_url);
		sconfig.channel_id = channel_id;
		sconfig.video = video;
		sconfig.video_codec_type = video_codec_type;
		sconfig.video_bit_rate = (int)video_bit_rate;
		sconfig.audio = audio;
		sconfig.audio_codec_type = audio_codec_type;
		sconfig.audio_bit_rate = (int)audio_bit_rate;
		// sconfig.audio_codec_lyra_params = config_.audio_codec_lyra_params;
		// sconfig.insecure = true;
		// sconfig.metadata = config_.metadata;
		// sconfig.network_manager =
		// 	connection_context_->default_network_manager();
		// sconfig.socket_factory =
		// 	connection_context_->default_socket_factory();
		signaling_ = sora::SoraSignaling::Create(std::move(sconfig));
		signaling_->Connect();
	}

	io_thread_ = rtc::Thread::Create();
	if (!io_thread_->SetName("Sora IO Thread", nullptr)) {
		RTC_LOG(LS_INFO) << "Failed to set thread name";
		// on_disconnect((int)sora::SoraSignalingErrorCode::INTERNAL_ERROR,
		// 				"Failed to set thread name");
		return;
	}
	if (!io_thread_->Start()) {
		RTC_LOG(LS_INFO) << "Failed to start thread";
		// on_disconnect((int)sora::SoraSignalingErrorCode::INTERNAL_ERROR,
		// 				"Failed to start thread");
		return;
	}
	io_thread_->PostTask([this]() {
		auto guard = boost::asio::make_work_guard(*ioc_);
		RTC_LOG(LS_INFO) << "io_context started";
		ioc_->run();
		RTC_LOG(LS_INFO) << "io_context finished";
	});
}

void SoraClient::OnSetOffer(std::string offer)
{
	UNUSED_PARAMETER(offer);
	std::string stream_id = rtc::CreateRandomString(16);
	if (audio_track_ != nullptr) {
		webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
			audio_result =
				signaling_->GetPeerConnection()->AddTrack(
					audio_track_, {stream_id});
	}
	if (video_track_ != nullptr) {
		webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
			video_result =
				signaling_->GetPeerConnection()->AddTrack(
					video_track_, {stream_id});
	}
}

void SoraClient::Disconnect()
{
	info("SoraClient::Disconnect");
	if (signaling_ == nullptr) {
		return;
	}
	signaling_->Disconnect();
}

void SoraClient::OnDisconnect(sora::SoraSignalingErrorCode ec,
			      std::string message)
{
	UNUSED_PARAMETER(ec);
	info("SoraClient::OnDisconnect '%s'", message.c_str());
	ioc_->stop();
}

// class CustomLogger : public rtc::LogSink {
// public:
// 	void OnLogMessage(const std::string &message) override
// 	{
// 		info("%s", message.c_str());
// 	}
// };

// CustomLogger logger;

ImageFluxOutput::ImageFluxOutput()
{
	// rtc::LogMessage::RemoveLogToStream(&logger);
	// rtc::LogMessage::AddLogToStream(&logger,
	// 				rtc::LoggingSeverity::LS_VERBOSE);
	// rtc::LogMessage::LogTimestamps();
}

ImageFluxOutput::~ImageFluxOutput()
{
}

bool ImageFluxOutput::start()
{
	sora_client = std::make_shared<SoraClient>();
	sora_client->output = output;
	sora_client->Connect();

	if (!obs_output_can_begin_data_capture(output, 0))
		return false;

	width_ = obs_output_get_width(output);
	height_ = obs_output_get_height(output);
	info("output width=%d, height=%d ***", width_, height_);

	struct video_scale_info vsi = {VIDEO_FORMAT_I420,0,0,VIDEO_RANGE_DEFAULT,VIDEO_CS_DEFAULT};
	obs_output_set_video_conversion(output, &vsi);

	struct audio_convert_info aci = {48000, AUDIO_FORMAT_16BIT, SPEAKERS_STEREO};
	obs_output_set_audio_conversion(output, &aci);		

	obs_output_begin_data_capture(output, 0);
	return true;
}

bool ImageFluxOutput::stop()
{
	sora_client->Disconnect();
	sora_client = nullptr;

	obs_output_end_data_capture(output);
	return true;
}

void ImageFluxOutput::audio(audio_data *frame)
{
	if (!sora_client)
		return;

	if (!frame)
		return;

	sora_client->audio_source_->OnAudioData(frame);
}

void ImageFluxOutput::video(video_data *frame)
{
	if (!sora_client)
		return;

	auto i420_buffer = webrtc::I420Buffer::Copy(width_, height_,
						    frame->data[0], width_,
						    frame->data[1], width_ / 2,
						    frame->data[2], width_ / 2);

	webrtc::VideoFrame video_frame =
		webrtc::VideoFrame::Builder()
			.set_video_frame_buffer(i420_buffer)
			.set_timestamp_rtp(0)
			.set_timestamp_ms(rtc::TimeMillis())
			.set_timestamp_us(rtc::TimeMicros())
			.set_rotation(webrtc::kVideoRotation_0)
			.build();
	sora_client->video_source_->OnCapturedFrame(video_frame);
}

uint64_t ImageFluxOutput::totalBytes()
{
	return 0;
}

int ImageFluxOutput::droppedFrames()
{
	return 0;
}

float ImageFluxOutput::congestion()
{
	return 0.0;
}
