#include "App.h"

int main(int argc, char** argv)
{
    std::string pathToConfig;
    switch (argc) {
    case 1:
        pathToConfig = std::string("../config.json");
        break;
    default:
        pathToConfig = std::string(argv[1]);
        break;
    }
    App app(pathToConfig);
    int result = app.Run();
    return result;
}
