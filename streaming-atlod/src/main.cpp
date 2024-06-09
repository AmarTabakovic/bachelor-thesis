#include "application.h"
#include <iostream>

int main(int argc, char** argv)
{
    Application::parseArgs(argc, argv);
    Application::setup();
    Application::run();

    return 0;
}
