#pragma once

// ---------------------------------------------------------------------------
// Network configuration
// ---------------------------------------------------------------------------

// Multicast group address (IPv4)
#define MULTICAST_IP    239, 0, 0, 1

// UDP port to listen on
#define MULTICAST_PORT  5004

// Maximum UDP payload size (bytes)
// Standard Ethernet MTU (1500) minus IP (20) and UDP (8) headers
#define UDP_PACKET_SIZE 1472

// ---------------------------------------------------------------------------
// Audio configuration
// ---------------------------------------------------------------------------

// Samples per audio block expected from the Teensy Audio library
// AUDIO_BLOCK_SAMPLES is defined by <Audio.h> and is 128 by default.
// This constant is provided here for documentation purposes only.
// #define AUDIO_BLOCK_SAMPLES 128   // defined by Audio.h

// Number of audio memory blocks allocated for the audio library.
// Each block is AUDIO_BLOCK_SAMPLES * 2 bytes = 256 bytes.
// 16 blocks give ~3 ms of audio buffer at 44100 Hz which is sufficient
// for direct (no-jitter-buffer) playback.
#define AUDIO_MEMORY_BLOCKS 16

// Number of I2S output channels used (1 = mono duplicated on both outputs,
// 2 = true stereo from left/right interleaved samples in the packet)
#define AUDIO_CHANNELS 1
