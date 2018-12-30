#include "util.hpp"

string color(Color color, string text){
  return string ()
    + "\033[" + std::to_string (30 + color) + "m"
    + text
    + "\033[m"
  ;
}
