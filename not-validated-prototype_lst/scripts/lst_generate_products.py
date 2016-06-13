#! /usr/bin/env python

'''
    File: lst_generate_products.py

    Purpose: Runs all of the sub-applications required to generate LST
             products.

    Project: Land Satellites Data Systems Science Research and Development
             (LSRD) at the USGS EROS

    License: NASA Open Source Agreement 1.3
'''

import os
import sys
import logging
import glob
from argparse import ArgumentParser
from ConfigParser import ConfigParser
from multiprocessing import Pool

import lst_utilities as util

from lst_grid_points import read_grid_points


def retrieve_command_line_arguments():
    """Read arguments from the command line

    Returns:
        args <arguments>: The arguments read from the command line
    """

    parser = ArgumentParser(description='Runs MODTRAN on a pre-determined'
                                        ' set of points')

    parser.add_argument('--xml',
                        action='store', dest='xml_filename',
                        required=False, default=None,
                        help='The XML metadata file to use')

    parser.add_argument('--keep-intermediate-products',
                        action='store_true', dest='keep_inter_products',
                        required=False, default=False,
                        help='Keep usefull intermediate products')

    parser.add_argument('--debug',
                        action='store_true', dest='debug',
                        required=False, default=False,
                        help='Output debug messages and/or keep debug data')

    parser.add_argument('--version',
                        action='store_true', dest='version',
                        required=False, default=False,
                        help='Reports the version of the software')

    args = parser.parse_args()

    # Report the version and exit
    if args.version:
        print(util.Version.version_text())
        sys.exit(0)  # EXIT SUCCESS

    return args


def get_cfg_file_path(filename):
    """Build the full path to the config file

    Args:
        filename <str>: The name of the file to append to the full path

    Raises:
        Exception(<str>)
    """

    # Use the users home directory as the base source directory for
    # configuration
    if 'HOME' not in os.environ:
        raise Exception('[HOME] not found in environment')
    home_dir = os.environ.get('HOME')

    # Build the full path to the configuration file
    config_path = os.path.join(home_dir, '.usgs', 'espa', filename)

    return config_path


def retrieve_cfg(cfg_filename):
    """Retrieve the configuration for the cron

    Returns:
        cfg <ConfigParser>: Configuration for ESPA cron

    Raises:
        Exception(<str>)
    """

    # Build the full path to the configuration file
    config_path = get_cfg_file_path(cfg_filename)

    if not os.path.isfile(config_path):
        raise Exception('Missing configuration file [{}]'
                        .format(config_path))

    # Create the object and load the configuration
    cfg = ConfigParser()
    cfg.read(config_path)

    return cfg


def determine_grid_points(xml_filename, data_path, debug):
    """Determines the grid points to utilize

    Args:
        xml_filename <str>: XML metadata filename
        data_path <str>: Directory for LST data files
        debug <bool>: Debug logging and processing
    """

    output = ''
    try:
        cmd = ['lst_determine_grid_points.py',
               '--xml', xml_filename,
               '--data_path', data_path]

        if debug:
            cmd.append('--debug')

        output = util.System.execute_cmd(' '.join(cmd))
    finally:
        if len(output) > 0:
            logger = logging.getLogger(__name__)
            logger.info(output)


def extract_auxiliary_narr_data(xml_filename, lst_aux_path, debug):
    """Determines the grid points to utilize

    Args:
        xml_filename <str>: XML metadata filename
        lst_aux_path <str>: Directory for the auxiliary data files
        debug <bool>: Debug logging and processing
    """

    output = ''
    try:
        cmd = ['lst_extract_auxiliary_narr_data.py',
               '--xml', xml_filename,
               '--lst_aux_path', lst_aux_path]

        if debug:
            cmd.append('--debug')

        output = util.System.execute_cmd(' '.join(cmd))
    finally:
        if len(output) > 0:
            logger = logging.getLogger(__name__)
            logger.info(output)


def build_modtran_input(xml_filename, data_path, debug):
    """Determines the grid points to utilize

    Args:
        xml_filename <str>: XML metadata filename
        data_path <str>: Directory for LST data files
        debug <bool>: Debug logging and processing
    """

    output = ''
    try:
        cmd = ['lst_build_modtran_input.py',
               '--xml', xml_filename,
               '--data_path', data_path]

        if debug:
            cmd.append('--debug')

        output = util.System.execute_cmd(' '.join(cmd))
    finally:
        if len(output) > 0:
            logger = logging.getLogger(__name__)
            logger.info(output)


def generate_emissivity_products(aster_ged_server_name):
    """Generate the required Emissivity products
    """

    # TODO TODO TODO
    # TODO TODO TODO
    # TODO TODO TODO
    # TODO TODO TODO
    # TODO TODO TODO
    # TODO TODO TODO
    pass


def run_modtran(modtran_data_path, process_count, debug):
    """Determines the grid points to utilize

    Args:
        modtran_data_path <str>: Directory for the MODTRAN 'DATA' files
        process_count <str>: Number of processes to use
        debug <bool>: Debug logging and processing
    """

    output = ''
    try:
        cmd = ['lst_run_modtran.py',
               '--modtran_data_path', modtran_data_path,
               '--process_count', process_count]

        if debug:
            cmd.append('--debug')

        output = util.System.execute_cmd(' '.join(cmd))
    finally:
        if len(output) > 0:
            logger = logging.getLogger(__name__)
            logger.info(output)


PROC_CFG_FILENAME = 'processing.conf'


def main():
    """Main processing for building the points list
    """

    # Command Line Arguments
    args = retrieve_command_line_arguments()

    # Check logging level
    logging_level = logging.INFO
    if args.debug:
        logging_level = logging.DEBUG

    # Setup the default logger format and level.  Log to STDOUT.
    logging.basicConfig(format=('%(asctime)s.%(msecs)03d %(process)d'
                                ' %(levelname)-8s'
                                ' %(filename)s:%(lineno)d:'
                                '%(funcName)s -- %(message)s'),
                        datefmt='%Y-%m-%d %H:%M:%S',
                        level=logging_level,
                        stream=sys.stdout)
    logger = logging.getLogger(__name__)

    logger.info('*** Begin LST Generate Products ***')

    # Retrieve the processing configuration
    proc_cfg = retrieve_cfg(PROC_CFG_FILENAME)

    # Determine number of process to use
    process_count = proc_cfg.get('processing', 'omp_num_threads')

    # Determine LST data locations
    data_path = proc_cfg.get('processing', 'lst_data_path')

    # Determine NARR data locations
    lst_aux_path = proc_cfg.get('processing', 'lst_aux_path')

    # Determine MODTRAN 'DATA' location
    modtran_data_path = proc_cfg.get('processing', 'modtran_data_path')

    # Determine the server name to get the ASTER data from
    aster_ged_server_name = proc_cfg.get('processing', 'aster_ged_server_name')

    # -------------- Generate the products --------------
    determine_grid_points(xml_filename=args.xml_filename,
                          data_path=data_path,
                          debug=args.debug)

    extract_auxiliary_narr_data(xml_filename=args.xml_filename,
                                lst_aux_path=lst_aux_path,
                                debug=args.debug)

    build_modtran_input(xml_filename=args.xml_filename,
                        data_path=data_path,
                        debug=args.debug)

    generate_emissivity_products(aster_ged_server_name)

    run_modtran(modtran_data_path=modtran_data_path,
                process_count=process_count,
                debug=args.debug)

    # TODO TODO TODO
    # TODO TODO TODO
    # Generate Intermediate Products
    # Generate Land Surface Temperature Product
    # TODO TODO TODO
    # TODO TODO TODO

    logger.info('*** LST Generate Products - Complete ***')


if __name__ == '__main__':
    main()