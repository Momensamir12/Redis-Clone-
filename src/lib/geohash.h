#ifndef GEOHASH_H
#define GEOHASH_H

#include <stdint.h>

/* Encode longitude and latitude into a 52-bit geohash */
uint64_t geohash_encode(double longitude, double latitude);

/* Decode 52-bit geohash into longitude and latitude */
void geohash_decode(uint64_t hash, double *longitude, double *latitude);

/* Validate geographic coordinates */
int geohash_validate_coordinates(double longitude, double latitude);

/* Encode geohash as a double score (preserves all bits) */
double geohash_encode_to_score(double longitude, double latitude);

/* Decode score back to geohash */
uint64_t geohash_decode_from_score(double score);

#endif