// Copyright (c) 2026 Kyle Bueche.
// Author: Kyle Bueche.
// This project is licensed under the MIT Licence - see LICENSE.txt.
// No warranty implied.

#include <iostream>
#include "vk_engine.h"

int main()
{
    VulkanEngine engine;
    engine.init();
    engine.run();
    engine.cleanup();
    return 0;
}