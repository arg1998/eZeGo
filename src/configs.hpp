#pragma once


#ifndef EZ_APPLICATION_VERSION_STR
    #define EZ_APPLICATION_VERSION_STR "0.0.0"
#endif

#ifndef EZ_APPLICATION_VERSION_NUM
    #define EZ_APPLICATION_VERSION_NUM 0L
#endif


/*
    This file contains all the compile configurations and flags 
*/


#define EZ_CONFIG_ASSERTION_ENABLED         true

// 1. On EZ_RELEASE_BUILD, debug and trace logs are turned off regardless of bellow option
// 2. Disabling this option will not affect EZ_LOG_FATAL() and EZ_LOG_ERROR()
#define EZ_CONFIG_LOGGING_ENBABLED          true


// define the size of the buffer that is used to extract variadic arguments
// this size represent the number of ASCII characters
#define EZ_CONFIG_LOG_BUFFER_SIZE           2048 * 10


// whether to spwan a separate console/terminal while running the GUI application
#define EZ_CONFIG_CONSOLE_ENABLED           false //TODO(Argosta): implement me 

// whether the log system should output filename only or file name + absolute path
#define EZ_CONFIG_LOG_FILE_ABSOLUTE_PATH    false
