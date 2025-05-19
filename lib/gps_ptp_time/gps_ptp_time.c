#include "gps_ptp_time.h"

// Global instance and mutex for thread-safe access
gps_ptp_time_t g_gps_ptp_time = {0};
SemaphoreHandle_t g_gps_ptp_time_mutex;

