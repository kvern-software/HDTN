/// \file util.hh
#pragma once

/// \cond DO_NOT_DOCUMENT
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/time.h>
#endif

// ssize_t definition for all systems
#if defined(_MSC_VER)
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif

#include <stdint.h>

/// \endcond

/**
 * \enum RTP_ERROR
 *
 * \brief RTP error codes
 *
 * \details These error valus are returned from various uvgRTP functions. Functions that return a pointer set rtp_errno global value that should be checked if a function call failed
 */
typedef enum RTP_ERROR {
    /// \cond DO_NOT_DOCUMENT
    RTP_MULTIPLE_PKTS_READY = 6,
    RTP_PKT_READY           = 5,
    RTP_PKT_MODIFIED        = 4,
    RTP_PKT_NOT_HANDLED     = 3,
    RTP_INTERRUPTED         = 2,
    RTP_NOT_READY           = 1,
    /// \endcond

    RTP_OK                  = 0,    ///< Success
    RTP_GENERIC_ERROR       = -1,   ///< Generic error condition
    RTP_SOCKET_ERROR        = -2,   ///< Failed to create socket
    RTP_BIND_ERROR          = -3,   ///< Failed to bind to interface
    RTP_INVALID_VALUE       = -4,   ///< Invalid value
    RTP_SEND_ERROR          = -5,   ///< System call send(2) or one of its derivatives failed
    RTP_MEMORY_ERROR        = -6,   ///< Memory allocation failed
    RTP_SSRC_COLLISION      = -7,   ///< SSRC collision detected
    RTP_INITIALIZED         = -8,   ///< Object already initialized
    RTP_NOT_INITIALIZED     = -9,   ///< Object has not been initialized
    RTP_NOT_SUPPORTED       = -10,  ///< Method/version/extension not supported
    RTP_RECV_ERROR          = -11,  ///< System call recv(2) or one of its derivatives failed
    RTP_TIMEOUT             = -12,  ///< Operation timed out
    RTP_NOT_FOUND           = -13,  ///< Object not found
    RTP_AUTH_TAG_MISMATCH   = -14,  ///< Authentication tag does not match the RTP packet contents
} rtp_error_t;

/**
 * \enum RTP_FORMAT
 *
 * \brief These flags are given to uvgrtp::session::create_stream()
 */
typedef enum RTP_FORMAT {
    // See RFC 3551 for more details

    // static audio profiles
    RTP_FORMAT_GENERIC    = 0,   ///< Same as PCMU
    RTP_FORMAT_PCMU       = 0,   ///< PCMU, ITU-T G.711
    // 1 is reserved in RFC 3551 
    // 2 is reserved in RFC 3551
    RTP_FORMAT_GSM        = 3,   ///< GSM (Group Speciale Mobile)
    RTP_FORMAT_G723       = 4,   ///< G723
    RTP_FORMAT_DVI4_32    = 5,   ///< DVI 32 kbit/s
    RTP_FORMAT_DVI4_64    = 6,   ///< DVI 64 kbit/s
    RTP_FORMAT_LPC        = 7,   ///< LPC
    RTP_FORMAT_PCMA       = 8,   ///< PCMA
    RTP_FORMAT_G722       = 9,   ///< G722
    RTP_FORMAT_L16_STEREO = 10,  ///< L16 Stereo
    RTP_FORMAT_L16_MONO   = 11,  ///< L16 Mono
    // 12 QCELP is unsupported in uvgRTP
    // 13 CN is unsupported in uvgRTP
    // 14 MPA is unsupported in uvgRTP
    RTP_FORMAT_G728       = 15,  ///< G728
    RTP_FORMAT_DVI4_441   = 16,  ///< DVI 44.1 kbit/s
    RTP_FORMAT_DVI4_882   = 17,  ///< DVI 88.2 kbit/s
    RTP_FORMAT_G729       = 18,  ///< G729, 8 kbit/s
    // 19 is reserved in RFC 3551
    // 20 - 23 are unassigned in RFC 3551

    /* static video profiles, unsupported in uvgRTP
    * 24 is unassigned
    * 25 is CelB, 
    * 26 is JPEG
    * 27 is unassigned
    * 28 is nv
    * 29 is unassigned
    * 30 is unassigned
    * 31 is H261
    * 32 is MPV
    * 33 is MP2T
    * 32 is H263
    */

    /* Rest of static numbers
    * 35 - 71 are unassigned
    * 72 - 76 are reserved
    * 77 - 95 are unassigned
    */
    
    /* Formats with dynamic payload numbers 96 - 127, including default values.
    * Use RCC_DYN_PAYLOAD_TYPE flag to change the number if desired. */

    RTP_FORMAT_G726_40   = 96,  ///< G726, 40 kbit/s
    RTP_FORMAT_G726_32   = 97,  ///< G726, 32 kbit/s
    RTP_FORMAT_G726_24   = 98,  ///< G726, 24 kbit/s
    RTP_FORMAT_G726_16   = 99,  ///< G726, 16 kbit/s
    RTP_FORMAT_G729D     = 100, ///< G729D, 6.4 kbit/s
    RTP_FORMAT_G729E     = 101, ///< G729E, 11.8 kbit/s
    RTP_FORMAT_GSM_EFR   = 102, ///< GSM enhanced full rate speech transcoding
    RTP_FORMAT_L8        = 103, ///< L8, linear audio data samples
    // RED is unsupported in uvgRTP
    RTP_FORMAT_VDVI      = 104, ///< VDVI, variable-rate DVI4
    RTP_FORMAT_OPUS      = 105, ///< Opus, see RFC 7587
    // H263-1998 is unsupported in uvgRTP
    RTP_FORMAT_H264      = 106, ///< H.264/AVC, see RFC 6184
    RTP_FORMAT_H265      = 107, ///< H.265/HEVC, see RFC 7798
    RTP_FORMAT_H266      = 108  ///< H.266/VVC
    
} rtp_format_t;



