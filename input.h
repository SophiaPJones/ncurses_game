#ifndef __INPUT_H_
#define __INPUT_H_
#include <unordered_set>
#include <stdio>
#include <ncurses.h>
class Input
{
private:
  std::unordered_set curkeys;
  std::unordered_set prevkeys;
  std::unordered_map keybindings;
public:
  void keyPressed(char key)
  {
    curkeys.
  }

};
#endif
