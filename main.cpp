/// TOUT CE CODE EST A MOI
/// RIEN QU'A MOI
/// DROIT ET LICENCE MOI

#include <iostream>

#include "src/CicadaEngine.h"

int main() {
    try {
        CicadaEngine engine;
        engine.run();
    } catch ( const std::exception &exception ) {
        std::cerr << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
