#ifndef _IMAGEFLUX_CLIENT_H_
#define _IMAGEFLUX_CLIENT_H_

#include "obs.h"

#include <api/peer_connection_interface.h>
#include <pc/connection_context.h>

#include <sora/sora_signaling.h>
#include <sora/scalable_track_source.h>

// OBSで生成したオーディオをWebRTCサーバに送るクラス
class OBSAudioSource : public webrtc::Notifier<webrtc::AudioSourceInterface> {
public:
	SourceState state() const override { return kLive; }
	bool remote() const override { return false; }
	void AddSink(webrtc::AudioTrackSinkInterface* sink) override { sink_ = sink; }
	void RemoveSink(webrtc::AudioTrackSinkInterface*) override { sink_ = nullptr; }
	const cricket::AudioOptions options() const override { return options_; };
	webrtc::AudioTrackSinkInterface *sink_;
	cricket::AudioOptions options_;

	// audio_t *audio_;
	uint16_t pending_remainder;
	uint8_t *pending;

	OBSAudioSource();
	~OBSAudioSource();

	void OnAudioData(audio_data *frame);
};

// 毎配信開始時に生成され、配信終了時に破棄されるクラス
class SoraClient : public std::enable_shared_from_this<SoraClient>,
		   public rtc::RefCountedObject<rtc::RefCountInterface>,
		   public sora::SoraSignalingObserver {
public:
	SoraClient();
	~SoraClient() override;
	obs_output_t *output;
	void Connect();
	void Disconnect();

	void OnSetOffer(std::string offer) override;
	void OnDisconnect(sora::SoraSignalingErrorCode ec,
			  std::string message) override;
	void OnNotify(std::string) override{};
	void OnPush(std::string) override{};
	void OnMessage(std::string, std::string) override{};
	void OnTrack(
		rtc::scoped_refptr<webrtc::RtpTransceiverInterface>) override{};
	void OnRemoveTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface>) override{};
	void OnDataChannel(std::string) override{};

private:
	std::unique_ptr<boost::asio::io_context> ioc_;
	std::shared_ptr<sora::SoraSignaling> signaling_;
	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track_;
	rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;

	std::unique_ptr<rtc::Thread> io_thread_;
	std::unique_ptr<rtc::Thread> network_thread_;
	std::unique_ptr<rtc::Thread> worker_thread_;
	std::unique_ptr<rtc::Thread> signaling_thread_;
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
	rtc::scoped_refptr<webrtc::ConnectionContext> connection_context_;

public:
	rtc::scoped_refptr<sora::ScalableVideoTrackSource> video_source_;
	rtc::scoped_refptr<OBSAudioSource> audio_source_;
	rtc::scoped_refptr<webrtc::AudioDeviceModule> adm_;
};

// 初回配信時に生成され、アプリ終了時に破棄されるクラス
class ImageFluxOutput {
public:
	ImageFluxOutput();
	~ImageFluxOutput();

	bool start();
	bool stop();
	void audio(audio_data *frame);
	void video(video_data *frame);

	uint64_t totalBytes();
	int droppedFrames();
	float congestion();

	std::shared_ptr<SoraClient> sora_client;

	int width_;
	int height_;

	obs_output_t *output;
};

#endif
