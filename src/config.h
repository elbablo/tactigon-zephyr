/* Event flags */
#define EVT_BT_CONNECTED            BIT(0)
#define EVT_BT_DISCONNECTED         BIT(1)
#define EVT_BT_SUB_CHAR_IMU         BIT(2)
#define EVT_BT_SUB_CHAR_AUDIO       BIT(3)
#define EVT_BT_UNSUB_CHAR_IMU       BIT(4)
#define EVT_BT_UNSUB_CHAR_AUDIO     BIT(5)
#define EVT_AUDIO_START             BIT(6)
#define EVT_IMU_START               BIT(7)

#define EVT_BT_ALL ( \
    EVT_BT_CONNECTED | EVT_BT_DISCONNECTED | \
    EVT_BT_SUB_CHAR_IMU | EVT_BT_SUB_CHAR_AUDIO | \
    EVT_BT_UNSUB_CHAR_IMU | EVT_BT_UNSUB_CHAR_AUDIO \
)

/* BLE config */
#define BLE_NAME                    "Tactigon-Skin"
#define BLE_BUFFER_LEN              20
#define BLE_THREAD_PRIORITY         4

/* Audio config */
#define PCM_SAMPLE_RATE             16000
#define PCM_WIDTH                   16
#define BLOCK_COUNT                 4
#define PCM_BLOCK_SIZE              512

/* IMU config */
#define G_VALUE                     9.80665f
#define FUSION_REFRESH_FREQ         104.0f
#define FUSION_BETA                 0.08f
#define M_PI                        3.14159265358979323846
