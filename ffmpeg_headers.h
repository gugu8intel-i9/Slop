// Simplified core FFmpeg structures representing Video packets and frames
struct AVCodecContext;
struct AVPacket;
struct AVFrame;

// Core FFmpeg FFI prototypes for video decoding and memory management
struct AVPacket* av_packet_alloc();
struct AVFrame* av_frame_alloc();
int avcodec_send_packet(struct AVCodecContext* ctx, struct AVPacket* pkt);
int avcodec_receive_frame(struct AVCodecContext* ctx, struct AVFrame* frame);
void av_packet_free(struct AVPacket** pkt);
void av_frame_free(struct AVFrame** frame);
