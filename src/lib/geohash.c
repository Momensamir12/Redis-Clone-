#include "geohash.h"
#include <stdint.h>
#include <string.h>

#define GEO_STEP_MAX 26
#define GEO_LAT_MIN -85.05112878
#define GEO_LAT_MAX 85.05112878
#define GEO_LONG_MIN -180
#define GEO_LONG_MAX 180

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
    /* Normalize coordinates to [0, 1] range */
    double lat_offset = (latitude - GEO_LAT_MIN) / (GEO_LAT_MAX - GEO_LAT_MIN);
    double lon_offset = (longitude - GEO_LONG_MIN) / (GEO_LONG_MAX - GEO_LONG_MIN);

    /* Scale to 26-bit integers */
    lat_offset *= (1ULL << GEO_STEP_MAX);
    lon_offset *= (1ULL << GEO_STEP_MAX);

    uint32_t lat_int = (uint32_t)lat_offset;
    uint32_t lon_int = (uint32_t)lon_offset;

    /* Redis interleaves as: interleave64(lat, lon) 
     * Result is [LAT][LONG] where LAT bits are in even positions */
    return interleave64(lat_int, lon_int);
}

void geohash_decode(uint64_t hash, double *longitude, double *latitude) {
    uint64_t hash_sep = deinterleave64(hash);
    
    /* After deinterleave: lower 32 bits are lat, upper 32 bits are lon */
    uint32_t lat_int = (uint32_t)(hash_sep & 0xFFFFFFFF);
    uint32_t lon_int = (uint32_t)((hash_sep >> 32) & 0xFFFFFFFF);

    /* Convert back to [0, 1] range */
    double lat_offset = (double)lat_int / (1ULL << GEO_STEP_MAX);
    double lon_offset = (double)lon_int / (1ULL << GEO_STEP_MAX);

    /* Scale back to geographic coordinates */
    *latitude = lat_offset * (GEO_LAT_MAX - GEO_LAT_MIN) + GEO_LAT_MIN;
    *longitude = lon_offset * (GEO_LONG_MAX - GEO_LONG_MIN) + GEO_LONG_MIN;
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
    
    /* Use union to reinterpret bits without numeric conversion */
    union {
        uint64_t u64;
        double d;
    } converter;
    
    converter.u64 = geohash;
    return converter.d;
}

uint64_t geohash_decode_from_score(double score) {
    /* Use union to reinterpret bits without numeric conversion */
    union {
        uint64_t u64;
        double d;
    } converter;
    
    converter.d = score;
    return converter.u64;
}