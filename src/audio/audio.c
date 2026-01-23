#include "audio/audio.h"

LOG_MODULE_REGISTER(audio);

/* Mem slab per PCM */
K_MEM_SLAB_DEFINE_STATIC(mem_slab, PCM_BLOCK_SIZE, BLOCK_COUNT, 4);

/* Thread */
K_THREAD_STACK_DEFINE(audio_stack, AUDIO_STACK_SIZE);
static struct k_thread audio_thread_data;

/* === DEVICE === */
const struct device *dmic_dev = DEVICE_DT_GET(DMIC_NODE);

/* === AUDIO CONFIG === */
static struct pcm_stream_cfg pcm_stream = {
    .pcm_width = PCM_WIDTH,
    .pcm_rate = PCM_SAMPLE_RATE,
    .block_size = PCM_BLOCK_SIZE,
    .mem_slab = &mem_slab,
};

static struct dmic_cfg dmic_cfg = {
    .io = {
        .min_pdm_clk_freq = 1000000,
        .max_pdm_clk_freq = 3000000,
        .min_pdm_clk_dc = 40,
        .max_pdm_clk_dc = 60,
    },
    .streams = &pcm_stream,
    .channel = {
        .req_num_streams = 1,
        .req_num_chan = 1,
    },
};

static struct adpcm_state adpcm_state;

int audio_init(void)
{
    if (!device_is_ready(dmic_dev))
    {
        LOG_ERR("DMIC device not ready");
        return -ENODEV;
    }

    int err = dmic_configure(dmic_dev, &dmic_cfg);
    if (err)
    {
        LOG_ERR("dmic_configure failed %d", err);
        return err;
    }

    audio_thread_init();
    LOG_INF("Audio subsystem ready");
    return 0;
}

/* Thread entry */
static void audio_thread(void *a, void *b, void *c)
{
    void *pcm_block;
    size_t pcm_size;

    while (1)
    {
        /* Attendi comando di start */
        k_event_wait(&app_events, EVT_AUDIO_START, false, K_FOREVER);

        LOG_INF("Audio streaming START");

        adpcm_init(&adpcm_state);
        dmic_trigger(dmic_dev, DMIC_TRIGGER_START);

        /* Loop streaming */
        while (k_event_wait(&app_events, EVT_AUDIO_START, false, K_NO_WAIT))
        {
            int ret = dmic_read(dmic_dev, 0, &pcm_block, &pcm_size, SYS_FOREVER_MS);
            if (ret)
            {
                LOG_ERR("dmic_read failed %d", ret);
                break;
            }

            LOG_DBG("Audio buffer ready");

            struct adpcm_packet block;
            block.len = adpcm_encode((int16_t *)pcm_block, pcm_size / sizeof(int16_t), block.buf, &adpcm_state);

            /* Invia il buffer pronto nella coda */
            if (k_msgq_put(&audio_queue, &block, K_NO_WAIT) != 0)
            {
                LOG_WRN("ADPCM queue full, buffer dropped");
            }
            k_mem_slab_free(pcm_stream.mem_slab, pcm_block);
        }

        dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
        LOG_INF("Audio streaming STOP");
    }
}

void audio_thread_init(void)
{
    k_thread_create(&audio_thread_data,
                    audio_stack,
                    K_THREAD_STACK_SIZEOF(audio_stack),
                    audio_thread,
                    NULL, NULL, NULL,
                    BLE_THREAD_PRIORITY,
                    0,
                    K_NO_WAIT);
}
