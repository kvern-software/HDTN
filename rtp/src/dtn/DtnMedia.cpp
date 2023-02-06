// #include "DtnMedia.h"

// rtp_error_t DtnMedia::PushFrame(buffer * image_buffer, int rtp_flags) 
// {
//     if (!image_buffer->start || !image_buffer->length)
//         return RTP_INVALID_VALUE;

//     return PushMediaFrame(image_buffer, rtp_flags);
// }


// // Pushed media frame into final queue to be bundled and sent
// rtp_error_t DtnMedia::PushMediaFrame(buffer * image_buffer, int rtp_flags)
// {
//     (void)rtp_flags;

//     rtp_error_t ret;

//     // if ((ret = fqueue_->init_transaction(data)) != RTP_OK) {
//     //     UVG_LOG_ERROR("Invalid frame queue or failed to initialize transaction!");
//     //     return ret;
//     // }

//     bool set_marker = false; // important M marker

//     if (image_buffer->length <= m_DtnContext->GetMTU()) {
//         // if ((ret = fqueue_->enqueue_message(data, data_len, set_marker)) != RTP_OK) {
//         //     UVG_LOG_ERROR("Failed to enqueue message: %d", ret);
//         //     (void)fqueue_->deinit_transaction();
//         //     return ret;
//         // }

//         // return fqueue_->flush_queue();
//     } else {
//         printf("Buffer larger than MTU");
//     }
// }