#ifndef __APS_WAV_FORMAT_H__
#define __APS_WAV_FORMAT_H__

typedef struct _s_wav_header {
    char        magic_riff[4];
    uint32_t    size_total;
    char        magic_wave[4];
    char        magic_fmt[4];
    uint32_t    chunk;
    uint16_t    fmt;
    uint16_t    ch;
    uint32_t    sampling;
    uint32_t    datarate;
    uint16_t    size_all_ch;
    uint16_t    bitwidth_ch;
    char        magic_data[4];
    uint32_t    size_payload;
} __attribute__((packed)) WavHeader;

typedef struct _s_wav_block {
    int16_t    l;
    int16_t    r;
} __attribute__((packed)) WavBlock;


#endif /* __APS_WAV_FORMAT_H__ */
