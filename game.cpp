#include <iostream>
#include <ncurses.h>
#include <unordered_set>
#include <thread>
#include <chrono>


int main()
{
  std::unordered_set<char> curinput;
  initscr();
  nodelay(stdscr, TRUE);
  raw();
  noecho();
  keypad(stdscr, TRUE);
  while(true) //gamerunning
    {
      char c;
      while((c = getch()) != ERR)
        {
          curinput.insert(ERR);
        }

      printw("currently pressed buttons: ");
      for(auto elem : curinput)
        {
          printw("%c",elem);
        }
      if(curinput.find((char)27) != curinput.end())
        {
          break;
        }
      refresh();
      curinput.clear();
      std::this_thread::sleep_for(1);
    }
  refresh();
  getch();
  endwin();
  return 1;
}
