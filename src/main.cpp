#include "App.h"

int main()
{
    App app("../config.json");
    int result = app.run();
    return result;
}
