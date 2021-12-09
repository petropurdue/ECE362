#ifndef __WAV2_H__
#define __WAV2_H__

typedef struct {
    uint32_t ChunkID;
    uint32_t ChunkSize;
    uint32_t Format;
    /// fmt
    uint32_t Subchunk1ID;
    uint32_t Subchunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
    /// data
    uint32_t Subchunk2ID;
    uint32_t Subchunk2Size;
} __attribute__((packed)) sWavHeader;

typedef struct header
{
    char fileformat[4];
    int file_size;
    char subformat[4];
    char subformat_id[4];
    int chunk_bits;                         // 16or18or40 due to pcm it is 16 here
    short int audio_format;                     // little or big endian
    short int num_channels;                     // 2 here for left and right
    int sample_rate;                        // sample_rate denotes the sampling rate.
    int byte_rate;                              // bytes  per second
    short int bytes_per_frame;
    short int bits_per_sample;
    char data_id[4];                        // "data" written in ascii
    int data_size;
}head;

void setup(sWavHeader *header, BYTE buffer);
int playwav(char * filename);

#endif
