#include <iostream>
#include "Engine.h"

int main()
{
    Engine app(1600, 720, (char*)"Vulkan", (char*)"0.0.0.1");

    try {
        
        app.run();
        
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}