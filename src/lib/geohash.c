#include "geohash.h"
#include <stdint.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GEO_STEP_MAX 26
#define GEO_LAT_MIN -85.05112878
#define GEO_LAT_MAX 85.05112878
#define GEO_LONG_MIN -180
#define GEO_LONG_MAX 180
#define EARTH_RADIUS_IN_METERS 6372797.560856

static inline uint64_t interleave64(uint32_t xlo, uint32_t ylo) {
    static const uint64_t B[] = {0x5555555555555555ULL, 0x3333333333333333ULL,
                                 0x0F0F0F0F0F0F0F0FULL, 0x00FF00FF00FF00FFULL,
                                 0x0000FFFF0000FFFFULL};
    static const unsigned int S[] = {1, 2, 4, 8, 16};

    uint64_t x = xlo;
    uint64_t y = ylo;

    x = (x | (x << S[4])) & B[4];
    y = (y | (y << S[4])) & B[4];

    x = (x | (x << S[3])) & B[3];
    y = (y | (y << S[3])) & B[3];

    x = (x | (x << S[2])) & B[2];
    y = (y | (y << S[2])) & B[2];

    x = (x | (x << S[1])) & B[1];
    y = (y | (y << S[1])) & B[1];

    x = (x | (x << S[0])) & B[0];
    y = (y | (y << S[0])) & B[0];

    return x | (y << 1);
}

static inline uint64_t deinterleave64(uint64_t interleaved) {
    static const uint64_t B[] = {0x5555555555555555ULL, 0x3333333333333333ULL,
                                 0x0F0F0F0F0F0F0F0FULL, 0x00FF00FF00FF00FFULL,
                                 0x0000FFFF0000FFFFULL, 0x00000000FFFFFFFFULL};
    static const unsigned int S[] = {0, 1, 2, 4, 8, 16};

    uint64_t x = interleaved;
    uint64_t y = interleaved >> 1;

    x = (x | (x >> S[0])) & B[0];
    y = (y | (y >> S[0])) & B[0];

    x = (x | (x >> S[1])) & B[1];
    y = (y | (y >> S[1])) & B[1];

    x = (x | (x >> S[2])) & B[2];
    y = (y | (y >> S[2])) & B[2];

    x = (x | (x >> S[3])) & B[3];
    y = (y | (y >> S[3])) & B[3];

    x = (x | (x >> S[4])) & B[4];
    y = (y | (y >> S[4])) & B[4];

    x = (x | (x >> S[5])) & B[5];
    y = (y | (y >> S[5])) & B[5];

    return x | (y << 32);
}

uint64_t geohash_encode(double longitude, double latitude) {
    double lat_offset = (latitude - GEO_LAT_MIN) / (GEO_LAT_MAX - GEO_LAT_MIN);
    double lon_offset = (longitude - GEO_LONG_MIN) / (GEO_LONG_MAX - GEO_LONG_MIN);

    lat_offset *= (1ULL << GEO_STEP_MAX);
    lon_offset *= (1ULL << GEO_STEP_MAX);

    uint32_t lat_int = (uint32_t)lat_offset;
    uint32_t lon_int = (uint32_t)lon_offset;

    return interleave64(lat_int, lon_int);
}

void geohash_decode(uint64_t hash, double *longitude, double *latitude) {
    uint64_t hash_sep = deinterleave64(hash);
    
    uint32_t lat_int = (uint32_t)(hash_sep & 0xFFFFFFFF);
    uint32_t lon_int = (uint32_t)((hash_sep >> 32) & 0xFFFFFFFF);

    /* Calculate the range for this grid cell */
    double lat_scale = GEO_LAT_MAX - GEO_LAT_MIN;
    double lon_scale = GEO_LONG_MAX - GEO_LONG_MIN;
    
    /* Get the minimum corner of the grid cell */
    double lat_min = GEO_LAT_MIN + (lat_int * 1.0 / (1ULL << GEO_STEP_MAX)) * lat_scale;
    double lon_min = GEO_LONG_MIN + (lon_int * 1.0 / (1ULL << GEO_STEP_MAX)) * lon_scale;
    
    /* Get the maximum corner of the grid cell */
    double lat_max = GEO_LAT_MIN + ((lat_int + 1) * 1.0 / (1ULL << GEO_STEP_MAX)) * lat_scale;
    double lon_max = GEO_LONG_MIN + ((lon_int + 1) * 1.0 / (1ULL << GEO_STEP_MAX)) * lon_scale;
    
    /* Return the center of the grid cell */
    *latitude = (lat_min + lat_max) / 2.0;
    *longitude = (lon_min + lon_max) / 2.0;
}

int geohash_validate_coordinates(double longitude, double latitude) {
    if (longitude < GEO_LONG_MIN || longitude > GEO_LONG_MAX) {
        return 0;
    }
    if (latitude < GEO_LAT_MIN || latitude > GEO_LAT_MAX) {
        return 0;
    }
    return 1;
}

double geohash_encode_to_score(double longitude, double latitude) {
    uint64_t geohash = geohash_encode(longitude, latitude);
    return (double)geohash;
}

uint64_t geohash_decode_from_score(double score) {
    return (uint64_t)score;
}

/* Convert degrees to radians */
static inline double deg_to_rad(double deg) {
    return deg * M_PI / 180.0;
}

/* Calculate distance between two points using Haversine formula */
double geohash_distance(double lon1, double lat1, double lon2, double lat2) {
    double lat1_rad = deg_to_rad(lat1);
    double lat2_rad = deg_to_rad(lat2);
    double lon1_rad = deg_to_rad(lon1);
    double lon2_rad = deg_to_rad(lon2);
    
    double dlat = lat2_rad - lat1_rad;
    double dlon = lon2_rad - lon1_rad;
    
    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(dlon / 2.0) * sin(dlon / 2.0);
    
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    
    return EARTH_RADIUS_IN_METERS * c;
}