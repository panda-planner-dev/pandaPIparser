#include "util.hpp"

string color(Color color, string text, Mode m){
  return string ()
    + "\033[" +std::to_string (m)+ ";" + std::to_string (30 + color) + "m"
    + text
    + "\033[0;m"
  ;
}
