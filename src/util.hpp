#include <string>

using namespace std;

enum Color{COLOR_GRAY,COLOR_RED,COLOR_GREEN,COLOR_YELLOW,COLOR_BLUE,COLOR_PURPLE,COLOR_CYAN};
enum Mode{MODE_NORMAL,MODE_BOLD,MODE_X,MODE_Y,MODE_Z,MODE_UNDERLINE};

string color (Color color, string text, Mode m = MODE_NORMAL);
