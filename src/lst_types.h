#ifndef LST_TYPES_H
#define LST_TYPES_H


#include <stdbool.h>
#include <limits.h>


typedef struct
{
    char path[PATH_MAX];
    char command[PATH_MAX];
    float latitude;
    float longitude;
    float height;
    bool run_modtran;
} MODTRAN_INFO;


typedef struct
{
    float ul_lat;
    float ul_lon;
    float lr_lat;
    float lr_lon;

    int min_row;
    int max_row;
    int min_col;
    int max_col;

    int num_rows;
    int num_cols;
    int num_points;
    int num_modtran_runs;

    MODTRAN_INFO *modtran_runs;

    float *row;
    float *col;
    float *lat;
    float *lon;
    float *utm_easting;
    float *utm_northing;
    bool *use_point;
} REANALYSIS_POINTS;


#endif /* LST_TYPES_H */
