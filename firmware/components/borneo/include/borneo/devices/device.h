
typedef struct {
    char* name;
    int (*init)(struct BorneoDevice* device);
    const void* config_info;
} BorneoDeviceConfig;

typedef struct {
    BorneoDeviceConfig* config;
    void* driver_api;
    void* driver_data;
} BorneoDevice;