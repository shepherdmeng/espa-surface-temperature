
#include <string.h>
#include <stdarg.h>
#include <time.h>


#include "const.h"
#include "2d_array.h"
#include "utilities.h"
#include "input.h"
#include "output.h"
#include "scene_based_lst.h"


/******************************************************************************
METHOD:  scene_based_lst

PURPOSE:  the main routine for scene based LST (Land Surface Temperature) in C

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           An error occurred during processing of the scene_based_lst
SUCCESS         Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
3/15/2013   Song Guo         Original Development

NOTES: type ./scene_based_lst --help for information to run the code
******************************************************************************/
int
main (int argc, char *argv[])
{
    char FUNC_NAME[] = "main";
    char msg_str[MAX_STR_LEN];  /* input data scene name */
    char *xml_name = NULL;      /* input XML filename */
    char *dem_name = NULL;      /* input DEM filename */
    char *emissivity_name = NULL;       /* input Emissivity filename */
    char directory[MAX_STR_LEN];        /* input/output data directory */
    char extension[MAX_STR_LEN];        /* input TOA file extension */
    Input_t *input = NULL;      /* input data and meta data */
    char scene_name[MAX_STR_LEN];       /* input data scene name */
    char command[PATH_MAX];
    int status;                 /* return value from function call */
    //    Output_t *output = NULL; /* output structure and metadata */
    bool use_tape6;             /* Use the tape6 output */
    bool verbose;               /* verbose flag for printing messages */
    bool debug;                 /* debug flag for debug output */
    Espa_internal_meta_t xml_metadata;  /* XML metadata structure */
    float alb = 0.1;
    int i;
    int num_points;
    int num_modtran_runs;
    char **case_list = NULL;
    char **command_list = NULL;
    float **results = NULL;
    char *tmp_env = NULL;

    time_t now;
    time (&now);
    snprintf (msg_str, sizeof(msg_str),
              "scene_based_lst start_time=%s", ctime (&now));
    LOG_MESSAGE (msg_str, FUNC_NAME);

    /* Read the command-line arguments, including the name of the input
       Landsat TOA reflectance product and the DEM */
    status = get_args (argc, argv, &xml_name, &dem_name, &emissivity_name,
                       &use_tape6, &verbose, &debug);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("calling get_args", FUNC_NAME, EXIT_FAILURE);
    }

    /* Verify the existence of required environment variables */
    /* Grab the environment path to the LST_DATA_DIR */
    tmp_env = getenv ("LST_DATA_DIR");
    if (tmp_env == NULL)
    {
        RETURN_ERROR ("LST_DATA_DIR environment variable is not set",
                      FUNC_NAME, EXIT_FAILURE);
    }

    /* Validate the input metadata file */
    if (validate_xml_file (xml_name) != SUCCESS)
    {
        /* Error messages already written */
        return EXIT_FAILURE;
    }

    /* Initialize the metadata structure */
    init_metadata_struct (&xml_metadata);

    /* Parse the metadata file into our internal metadata structure; also
       allocates space as needed for various pointers in the global and band
       metadata */
    if (parse_metadata (xml_name, &xml_metadata) != SUCCESS)
    {
        /* Error messages already written */
        return EXIT_FAILURE;
    }

    /* Split the filename to obtain the directory, scene name, and extension */
    split_filename (xml_name, directory, scene_name, extension);
    if (verbose)
    {
        printf ("directory, scene_name, extension=%s,%s,%s\n",
                directory, scene_name, extension);
    }

    /* Open input file, read metadata, and set up buffers */
    input = OpenInput (&xml_metadata);
    if (input == NULL)
    {
        RETURN_ERROR ("opening input files", FUNC_NAME, EXIT_FAILURE);
    }

    if (verbose)
    {
        /* Print some info to show how the input metadata works */
        printf ("Instrument: %d\n", input->meta.inst);
        printf ("Satellite: %d\n", input->meta.sat);
        printf ("Number of input thermal bands: %d\n",
                input->nband_th);
        printf ("Number of input lines: %d\n", input->size_th.l);
        printf ("Number of input samples: %d\n", input->size_th.s);
        printf ("ACQUISITION_DATE.DOY is %d\n",
                input->meta.acq_date.doy);
        printf ("Fill value is %d\n", input->meta.fill);
        printf ("Thermal Band -->\n");
        printf ("  therm_satu_value_ref: %d\n",
                input->meta.therm_satu_value_ref);
        printf ("  therm_satu_value_max: %d\n",
                input->meta.therm_satu_value_max);
        printf ("  therm_gain: %f, therm_bias: %f\n",
                input->meta.gain_th, input->meta.bias_th);

        printf ("SUN AZIMUTH: %f\n", input->meta.sun_az);
        printf ("SUN ZENITH: %f\n", input->meta.sun_zen);
        printf ("Year, Month, Day, Hour, Minute, Second: %d, "
                "%d, %d, %d, %d,%f\n", input->meta.acq_date.year,
                input->meta.acq_date.month, input->meta.acq_date.day,
                input->meta.acq_date.hour, input->meta.acq_date.minute,
                input->meta.acq_date.second);
        printf ("UL_MAP_CORNER: %f, %f\n", input->meta.ul_map_corner.x,
                input->meta.ul_map_corner.y);
        printf ("LR_MAP_CORNER: %f, %f\n", input->meta.lr_map_corner.x,
                input->meta.lr_map_corner.y);
        printf ("UL_GEO_CORNER: %f, %f\n",
                input->meta.ul_geo_corner.lat, input->meta.ul_geo_corner.lon);
        printf ("LR_GEO_CORNER: %f, %f\n",
                input->meta.lr_geo_corner.lat, input->meta.lr_geo_corner.lon);
    }

#if 0
    /* If the scene is an ascending polar scene (flipped upside down), then
       the solar azimuth needs to be adjusted by 180 degrees.  The scene in
       this case would be north down and the solar azimuth is based on north
       being up clock-wise direction. Flip the south to be up will not change 
       the actual sun location, with the below relations, the solar azimuth
       angle will need add in 180.0 for correct sun location */
    if (input->meta.ul_corner.is_fill &&
        input->meta.lr_corner.is_fill &&
        (input->meta.ul_corner.lat - input->meta.lr_corner.lat) < MINSIGMA)
    {
        /* Keep the original solar azimuth angle */
        sun_azi_temp = input->meta.sun_az;
        input->meta.sun_az += 180.0;
        if ((input->meta.sun_az - 360.0) > MINSIGMA)
            input->meta.sun_az -= 360.0;
        if (verbose)
            printf
                ("  Polar or ascending scene.  Readjusting solar azimuth by "
                 "180 degrees.\n  New value: %f degrees\n",
                 input->meta.sun_az);
    }
#endif


    /* call build_modtran_input to generate tape5 file and commandList */
    status = build_modtran_input (input, &num_points, &num_modtran_runs,
                                  &case_list, &command_list,
                                  verbose, debug);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Building MODTRAN input\n", FUNC_NAME, EXIT_FAILURE);
    }

    if (verbose)
    {
        printf ("Number of Points: %d\n", num_points);
    }

#if 0
TEMP TAKE THIS OUT TO SAVE TIME
    /* perform modtran runs by calling command_list */
    for (i = 0; i < num_modtran_runs; i++)
    {
        snprintf (msg_str, sizeof(msg_str),
                  "Executing MODTRAN [%s]", command_list[i]);
        LOG_MESSAGE (msg_str, FUNC_NAME);
        status = system (command_list[i]);
        if (status != SUCCESS)
        {
            RETURN_ERROR ("Error executing MODTRAN", FUNC_NAME,
                          EXIT_FAILURE);
        }
    }
#endif

    /* Free memory allocation */
    status = free_2d_array ((void **) command_list);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: command_list\n", FUNC_NAME,
                      EXIT_FAILURE);
    }

    if (use_tape6)
    {
        /* PARSING TAPE6 FILES:
           for each case in caseList (for each modtran run),
           parse wavelength and total radiance from tape6 file into parsed */
        for (i = 0; i < num_modtran_runs; i++)
        {
            snprintf (command, sizeof (command),
                      "lst_extract_tape6_results.py"
                      " --tape6 %s/tape6"
                      " --parsed %s/parsed",
                      case_list[i], case_list[i]);
            snprintf (msg_str, sizeof(msg_str),
                      "Executing [%s]", command);
            LOG_MESSAGE (msg_str, FUNC_NAME);
            status = system (command);
            if (status != SUCCESS)
            {
                RETURN_ERROR ("Failed executing lst_extract_tape6_results.py",
                              FUNC_NAME, EXIT_FAILURE);
            }
        }
    }
// TODO TODO TODO - Else use the pltout.asc since MODTRAN 5.X generates it and it  has more precision.

    /* Allocate memory for results */
    results = (float **) allocate_2d_array (num_points * NUM_ELEVATIONS, 6,
                                            sizeof (float));
    if (results == NULL)
    {
        RETURN_ERROR ("Allocating results memory", FUNC_NAME, EXIT_FAILURE);
    }

// TODO TODO TODO - RDD - stopping here for right now
// TODO TODO TODO - RDD - stopping here for right now
// TODO TODO TODO - RDD - stopping here for right now
// TODO TODO TODO - RDD - stopping here for right now
    exit (0);
// TODO TODO TODO - RDD - stopping here for right now
// TODO TODO TODO - RDD - stopping here for right now
// TODO TODO TODO - RDD - stopping here for right now
// TODO TODO TODO - RDD - stopping here for right now

    /* call second_narr to generate parameters for each height and NARR point */
    status =
        second_narr (input, num_points, alb, case_list, results, verbose);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Calling scene_based_list\n", FUNC_NAME, EXIT_FAILURE);
    }

    /* Free memory allocation */
    status = free_2d_array ((void **) case_list);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: current_case\n", FUNC_NAME,
                      EXIT_FAILURE);
    }

    /* call third_pixels_post to generate parameters for each Landsat pixel */
    status = third_pixels_post (input, num_points, dem_name, emissivity_name,
                                results, verbose);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Calling scene_based_list\n", FUNC_NAME, EXIT_FAILURE);
    }

#if 0
    /* Reassign solar azimuth angle for output purpose if south up north 
       down scene is involved */
    if (input->meta.ul_corner.is_fill &&
        input->meta.lr_corner.is_fill &&
        (input->meta.ul_corner.lat - input->meta.lr_corner.lat) < MINSIGMA)
    {
        input->meta.sun_az = sun_azi_temp;
    }

    /* Open the output file */
    output = OpenOutput (&xml_metadata, input);
    if (output == NULL)
    {                           /* error message already printed */
        RETURN_ERROR ("Opening output file", FUNC_NAME, EXIT_FAILURE);
    }

    if (!PutOutput (output, pixel_mask))
    {
        RETURN_ERROR ("Writing output LST in HDF files\n", FUNC_NAME,
                      EXIT_FAILURE);
    }

    /* Close the output file */
    if (!CloseOutput (output))
    {
        RETURN_ERROR ("closing output file", FUNC_NAME, EXIT_FAILURE);
    }

    /* Create the ENVI header file this band */
    if (create_envi_struct (&output->metadata.band[0], &xml_metadata.global,
                            &envi_hdr) != SUCCESS)
    {
        RETURN_ERROR ("Creating ENVI header structure.", FUNC_NAME,
                      EXIT_FAILURE);
    }

    /* Write the ENVI header */
    strcpy (envi_file, output->metadata.band[0].file_name);
    cptr = strchr (envi_file, '.');
    if (cptr == NULL)
    {
        RETURN_ERROR ("error in ENVI header filename", FUNC_NAME,
                      EXIT_FAILURE);
    }

    strcpy (cptr, ".hdr");
    if (write_envi_hdr (envi_file, &envi_hdr) != SUCCESS)
    {
        RETURN_ERROR ("Writing ENVI header file.", FUNC_NAME, EXIT_FAILURE);
    }

    /* Append the LST band to the XML file */
    if (append_metadata (output->nband, output->metadata.band, xml_name)
        != SUCCESS)
    {
        RETURN_ERROR ("Appending spectral index bands to XML file.",
                      FUNC_NAME, EXIT_FAILURE);
    }

    /* Free the structure */
    if (!FreeOutput (output))
    {
        RETURN_ERROR ("freeing output file structure", FUNC_NAME,
                      EXIT_FAILURE);
    }

    /* Free the metadata structure */
    free_metadata (&xml_metadata);

    /* Close the input file and free the structure */
    CloseInput (input);
    FreeInput (input);

    free (xml_name);
    printf ("Processing complete.\n");
#endif

    /* Free memory allocation */
    status = free_2d_array ((void **) results);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: results\n", FUNC_NAME, EXIT_FAILURE);
    }

#if 0
    /* Delete temporary file */
    status = system ("rm newHead*");
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Deleting newHead* files\n", FUNC_NAME, EXIT_FAILURE);
    }

    status = system ("rm newTail*");
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Deleting newTail* files\n", FUNC_NAME, EXIT_FAILURE);
    }

    status = system ("rm tempLayers.txt");
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Deleting tempLayers file\n", FUNC_NAME, EXIT_FAILURE);
    }

    /* Delete temporary directories */
    status = system ("\rm -r HGT*");
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Deleting HGT* directories\n", FUNC_NAME, EXIT_FAILURE);
    }

    status = system ("\rm -r SHUM*");
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Deleting SHUM* directories\n", FUNC_NAME, EXIT_FAILURE);
    }

    status = system ("\rm -r TMP*");
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Deleting TMP* directories\n", FUNC_NAME, EXIT_FAILURE);
    }

    status = system ("\rm -r 4?.*_*");
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Deleting temporary directories\n", FUNC_NAME,
                      EXIT_FAILURE);
    }
#endif

    time (&now);
    snprintf (msg_str, sizeof(msg_str),
              "scene_based_lst end_time=%s\n", ctime (&now));
    LOG_MESSAGE (msg_str, FUNC_NAME);

    return EXIT_SUCCESS;
}
