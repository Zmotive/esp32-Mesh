#ifndef GPS_PTP_TIME_H
#define GPS_PTP_TIME_H

#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief Structure to hold GPS time information for PTP-like synchronization.
 *
 * Contains the GPS iTOW (us of GPS week), tAcc (us), and the local system time (us since boot)
 * when the NAV-PVT iTOW was last updated.
 */
typedef struct gps_ptp_time_t{
    uint64_t gps_iTOW_us;   // GPS time of week in microseconds (iTOW*1000 + nano/1000, rounded)
    uint64_t updated_us;    // Local system time in us when iTOW was updated
    uint32_t tAcc_us;       // Time accuracy in microseconds (from NAV-PVT, rounded)
} gps_ptp_time_t;

extern gps_ptp_time_t g_gps_ptp_time;
extern SemaphoreHandle_t g_gps_ptp_time_mutex;

#endif // GPS_PTP_TIME_H
