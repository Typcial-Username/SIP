#include "audio.h"

void cosine() {
    dac_cosine_handle_t chan_handle;
    dac_cosine_config_t cos_cfg = {
        .chan_id = DAC_CHAN_0, // GPIO25 = DAC_CHAN_0, GPIO26 = DAC_CHAN_1
        .freq_hz = 40000,
        .clk_src = DAC_COSINE_CLK_SRC_DEFAULT,
        .atten = DAC_COSINE_ATTEN_DEFAULT,
        .phase = DAC_COSINE_PHASE_0,
        .offset = 0,
        .flags={.force_set_freq = false},
    };

    dac_cosine_new_channel(&cos_cfg, &chan_handle);
    dac_cosine_start(chan_handle);
}