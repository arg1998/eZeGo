#include "core/definitions.hpp"
#include "core/platform/platform.hpp"
#include "core/logger/logger.hpp"
#include "core/assertion/assertions.hpp"
#include "core/serial/SerialInterface.hpp"
void PLATFORM_MAIN() {
    EZ_LOG_TRACE();

    EZ_LOG_FATAL("fatal log %d %s", 123, " </fatal>");
    EZ_LOG_ERROR("error log %d %s", 456, "</error>");
    EZ_LOG_WARN("warn log %d %d %s", 789, false, "</warn>");
    EZ_LOG_INFO("info log %d %s", 123456, "</info>");
    EZ_LOG_DEBUG("debug log %d %s", 456789, "</debug>");

    EZ_ASSERT(1 == 1);
    EZ_ASSERT_MSG(1 == 1, "why 1 is not equal to 0?");

    SerialPortConfig config;
    config.port_name = "/dev/tty.usbmodem11303"; // Replace with your device path (e.g., /dev/tty.usbserial-XXXXX)
    config.baud_rate = 115200;
    config.character_size = 8;
    config.stop_bits = 0; // One stop bit
    config.parity = 0;    // No parity

    // TODO: Farid added for testing
    initiliazeSerialIO();
    openIOService(config);
    runBenchmarkIO(config, 512, 1);
    shutdownSerialIO();

    PlatformState *plat_state;

    plat_state = initPlatform("eZeGo");

    /* Make the window's context current */
    glfwMakeContextCurrent(plat_state->main_window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(plat_state->main_window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(plat_state->main_window);

        /* Poll for and process events */
        glfwPollEvents();
        platformSleep(10);
    }

    shutdownPlatform();

}