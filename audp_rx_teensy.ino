/**
 * audp_rx_teensy.ino
 *
 * Multicast UDP audio receiver for Teensy 4.1.
 *
 * The sketch joins an IPv4 multicast group, receives raw 16-bit signed PCM
 * packets over UDP and feeds the samples directly into the Teensy Audio
 * library via AudioPlayQueue.  There is deliberately NO jitter-buffer
 * management: samples are forwarded to the audio hardware as soon as each
 * UDP packet arrives.  If no packet arrives in time, silence is output
 * naturally by the audio library.
 *
 * Requirements
 * ------------
 *   Hardware : Teensy 4.1 with the built-in Ethernet port connected.
 *   Libraries:
 *     - QNEthernet  (https://github.com/ssilverman/QNEthernet)
 *     - Teensy Audio Library (bundled with Teensyduino)
 *
 * Audio format expected in each UDP packet
 * -----------------------------------------
 *   Raw 16-bit signed PCM, little-endian.
 *   Sample rate : 44100 Hz (Teensy Audio default).
 *   Channels    : configured via AUDIO_CHANNELS in config.h
 *                 1 = mono  (single AudioPlayQueue, duplicated on L+R)
 *                 2 = stereo (two AudioPlayQueues, interleaved L/R)
 *   Packet size : up to UDP_PACKET_SIZE bytes (see config.h).
 */

#include <QNEthernet.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>

#include "config.h"

using namespace qindesign::network;

// ---------------------------------------------------------------------------
// Audio graph  (mono — single queue duplicated on both I2S channels)
// ---------------------------------------------------------------------------
// This implementation supports AUDIO_CHANNELS == 1 (mono).
// Each received UDP packet is expected to carry single-channel 16-bit PCM
// samples; the same stream is routed to both Left and Right I2S outputs.
AudioPlayQueue  audioQueue;
AudioOutputI2S  audioOutput;

AudioConnection patchCordL(audioQueue, 0, audioOutput, 0);  // queue → Left
AudioConnection patchCordR(audioQueue, 0, audioOutput, 1);  // queue → Right

// ---------------------------------------------------------------------------
// Network
// ---------------------------------------------------------------------------
EthernetUDP udp;

// Receive buffer sized for the largest expected UDP payload.
// Aligned to int16_t to avoid any unaligned-access concern when casting
// the byte buffer to a 16-bit sample pointer.
static uint8_t packetBuffer[UDP_PACKET_SIZE] __attribute__((aligned(2)));

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) { /* wait for USB-Serial */ }

    Serial.println(F("audp_rx_teensy — multicast audio receiver"));
    Serial.println(F("=========================================="));

    // ----- Audio -----
    AudioMemory(AUDIO_MEMORY_BLOCKS);
    Serial.println(F("Audio memory allocated."));

    // ----- Ethernet (DHCP) -----
    Serial.println(F("Starting Ethernet (DHCP)…"));
    if (!Ethernet.begin()) {
        Serial.println(F("ERROR: Failed to start Ethernet."));
        while (true) { /* halt */ }
    }
    Serial.print(F("IP address : "));
    Serial.println(Ethernet.localIP());

    // ----- Multicast UDP -----
    IPAddress multicastGroup(MULTICAST_IP);
    Serial.print(F("Joining multicast group "));
    Serial.print(multicastGroup);
    Serial.print(F(" on port "));
    Serial.println(MULTICAST_PORT);

    if (!udp.beginMulticast(multicastGroup, MULTICAST_PORT)) {
        Serial.println(F("ERROR: Failed to join multicast group."));
        while (true) { /* halt */ }
    }

    Serial.println(F("Listening…"));
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop() {
    int packetSize = udp.parsePacket();
    if (packetSize <= 0) {
        return;  // No packet available — audio library will output silence.
    }

    // Clamp to our buffer size to avoid overflow.
    if (packetSize > UDP_PACKET_SIZE) {
        packetSize = UDP_PACKET_SIZE;
    }

    int bytesRead = udp.read(packetBuffer, packetSize);
    if (bytesRead <= 0) {
        return;
    }

    // Interpret payload as 16-bit signed PCM samples.
    const int16_t *samples    = reinterpret_cast<const int16_t *>(packetBuffer);
    const int      sampleCount = bytesRead / sizeof(int16_t);

    // Feed samples into the Teensy Audio queue one block at a time.
    // AUDIO_BLOCK_SAMPLES is defined by Audio.h (128 by default).
    for (int offset = 0; offset < sampleCount; offset += AUDIO_BLOCK_SAMPLES) {
        int16_t *block = audioQueue.getBuffer();
        if (block == nullptr) {
            // Queue is full — drop remaining samples from this packet.
            // No retry / jitter-buffer logic by design.
            break;
        }

        int available = sampleCount - offset;
        int toCopy    = (available >= AUDIO_BLOCK_SAMPLES)
                            ? AUDIO_BLOCK_SAMPLES
                            : available;

        memcpy(block, samples + offset, toCopy * sizeof(int16_t));

        // Zero-fill the remainder of the block if the packet data ran short.
        if (toCopy < AUDIO_BLOCK_SAMPLES) {
            memset(block + toCopy, 0,
                   (AUDIO_BLOCK_SAMPLES - toCopy) * sizeof(int16_t));
        }

        audioQueue.playBuffer();
    }
}
