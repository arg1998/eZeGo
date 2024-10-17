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

// whether log FATAL and ERROR to std::err stream or not. 
#define EZ_CONFIG_LOG_TO_STD_ERR            false

// whether to spwan a separate console/terminal while running the application
#define EZ_CONFIG_CONSOLE_ENABLED           false

// whether the log system should output filename only or file name + absolute path
#define EZ_CONFIG_LOG_FILE_ABSOLUTE_PATH    false
