// Per-OS process entry point. Compiled into executables only (never into ez_core), so tests and
// other executables can link the core and bring their own main() (linking.md L-6).
#include "platform.hpp"

// TODO(Argosta): WinMain for a console-less GUI build on Windows; macOS app-bundle entry.
int main(int argc, char** argv) {
    return PLATFORM_MAIN(argc, argv);
}
