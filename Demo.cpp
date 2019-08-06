#include <ncurses.h>
#include <panel.h>
#include <vector>
#include <deque>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>
#include <random>

enum base_colors_t {WHITE, RED, YELLOW, GREEN, CYAN, MAGENTA};

enum dir_t {UP, DOWN, LEFT, RIGHT};

struct object {
  object(size_t h, size_t w, size_t y, size_t x, PANEL* pan)
    : height(h), width(w), y(y), x(x), pan(pan) {}
  object() {}
  size_t height, width, y, x;
  PANEL* pan;
};

struct user : object {
  const char* str = "   ^\n  / \\\n<|___|>\0";
  user() : object(3, 7, 0, 0, nullptr) {}
} usr;

typedef struct projectile : object {
  projectile() {pan = nullptr;}
  projectile(size_t h, size_t w, size_t y, size_t x, PANEL* pan)
    : object(h, w, y, x, pan) {}
}projectile;

struct enemy : object {
  enemy() {}
  enemy(size_t h, size_t w, size_t y, size_t x, PANEL* pan)
    : object(h, w, y, x, pan) {}
  const char* str = "~\\o/~\n .v.\0";
  projectile proj;
  bool dead;
} cpu;

void init_ncurses();

size_t init_colors();

void init_player(struct user* plyr = &usr);

void init_enemy(struct enemy* enmy = &cpu);

void move_player(dir_t dir, struct user* plyr = &usr);

projectile create_projectile(size_t y, size_t x);

void move_projectiles(std::vector<projectile>& projectiles);

void destroy_projectile(projectile& projectile);

void update_enemy_loc(struct enemy& enmy);

bool obj_collision(struct object& o1, struct object& o2);

void enmy_fire_projectile(struct enemy& enmy);

void enmy_move_projectile(struct enemy& enmy, int& cycle);

void enmy_flash_death(struct enemy& enmy);

void destroy_object(struct object& o);

int main() {
  std::deque<projectile> projectiles(7);
  const int SPACE_BAR = ' ';
  int usr_input, cycleNum = 0;
  auto start = std::chrono::steady_clock::now();


  init_ncurses();
  init_colors();
  init_player();
  init_enemy();

  while ((usr_input = getch()) != KEY_F(1)) {
    switch(usr_input) {
    case KEY_UP:
      move_player(UP);
      break;
    case KEY_DOWN:
      move_player(DOWN);
      break;
    case KEY_LEFT:
      move_player(LEFT);
      break;
    case KEY_RIGHT:
      move_player(RIGHT);
      break;
    case SPACE_BAR:
      if (projectiles.size() < 8) {
        projectiles.emplace_back(create_projectile(usr.y-1, usr.x+(usr.width/2)));
        bottom_panel(projectiles.back().pan);
        update_panels();
        projectiles.shrink_to_fit();
      }
      break;
    }
    if (projectiles.size()) {
      for (auto& proj : projectiles) {
        std::this_thread::sleep_for (std::chrono::milliseconds(5));
        if (proj.y > 0) {
          move_panel(proj.pan, --proj.y, proj.x);
          if (cpu.pan && !panel_hidden(cpu.pan) && obj_collision(proj, cpu)) {
            cpu.dead = true;
            hide_panel(proj.pan);
          }
        }
        else {
          delwin(panel_window(proj.pan));
          del_panel(proj.pan);
          projectiles.pop_front();
        }
        projectiles.shrink_to_fit();
      }
    }
    if (cpu.pan && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count() > 150) {
      // Reset clock
      start = std::chrono::steady_clock::now();
      if (!cpu.dead)
        update_enemy_loc(cpu);
      else if (!panel_hidden(cpu.pan))
        enmy_flash_death(cpu);

      if (cpu.proj.pan)
        enmy_move_projectile(cpu, cycleNum);
      else if (!cpu.dead)
        enmy_fire_projectile(cpu);
      else
        destroy_object(cpu);
    }
    else if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-start).count() > 2)
      init_enemy();
    update_panels();
    doupdate();
  }

  endwin();

  return 0;
}

void init_ncurses() {
  initscr();
  start_color();
  cbreak();
  noecho();
  nodelay(stdscr, true);
  // turn cursor off
  curs_set(false);
  keypad(stdscr, true);
}

size_t init_colors() {
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_GREEN, COLOR_BLACK);
  init_pair(4, COLOR_CYAN, COLOR_BLACK);
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
  return 5;
}

void init_player(struct user* plyr) {
  // Set usr start position; horizontally centered, vertically right above
  // bottom of screen
  plyr->y = LINES-5;
  plyr->x = (COLS-plyr->width)/2;

  plyr->pan = new_panel(newwin(plyr->height, plyr->width, plyr->y, plyr->x));

  wattron(panel_window(plyr->pan), COLOR_PAIR(YELLOW));
  wprintw(panel_window(plyr->pan), plyr->str);
  wattroff(panel_window(plyr->pan), COLOR_PAIR(YELLOW));
}

void init_enemy(struct enemy* enmy) {
  enmy->height = 3;
  enmy->width = 5;
  enmy->y = enmy->height+1;
  enmy->x = (COLS-enmy->width)/2;
  enmy->pan = new_panel(newwin(enmy->height, enmy->width, enmy->y, enmy->x));
  enmy->dead = false;
  wattron(panel_window(enmy->pan), COLOR_PAIR(RED));
  wprintw(panel_window(enmy->pan), enmy->str);
  wattroff(panel_window(enmy->pan), COLOR_PAIR(RED));
}

void move_player(dir_t dir, struct user* plyr) {
  switch(dir) {
  case UP:
    if (plyr->y-1 > 0) --plyr->y;
    break;
  case DOWN:
    if (plyr->y+1 < (size_t)LINES+plyr->height) ++plyr->y;
    break;
  case LEFT:
    if (plyr->x-1 > 0) --plyr->x;
    break;
  case RIGHT:
    if (plyr->x+1 < (size_t)COLS-plyr->width-1) ++plyr->x;
    break;
  }
  wclear(panel_window(plyr->pan));
  wattron(panel_window(plyr->pan), COLOR_PAIR(YELLOW));
  move_panel(plyr->pan, plyr->y, plyr->x);
  wprintw(panel_window(plyr->pan), plyr->str);
  wattroff(panel_window(plyr->pan), COLOR_PAIR(YELLOW));
}

projectile create_projectile(size_t y, size_t x) {
  projectile ret (1, 1, y, x, new_panel(newwin(1, 1, y, x)));
  waddch(panel_window(ret.pan), '^');
  return ret;
}

void update_enemy_loc(struct enemy& enmy) {
  std::random_device rd;
  bool in_bounds = false;
  while (!in_bounds) {
    dir_t enmy_dir = (dir_t)(rd() % 4);
    switch(enmy_dir) {
    case UP:
      if (enmy.y-1 > 0) {
        --enmy.y;
        in_bounds = true;
      }
      break;
    case DOWN:
      if (enmy.y+1 < (size_t)LINES) {
        ++enmy.y;
        in_bounds = true;
      }
      break;
    case LEFT:
      if (enmy.x-1 > 0) {
        --enmy.x;
        in_bounds = true;
      }
      break;
    case RIGHT:
      if (enmy.x+1 < (size_t)COLS+(2*enmy.height)) {
        ++enmy.x;
        in_bounds = true;
      }
      break;
    }
  }
  move_panel(enmy.pan, enmy.y, enmy.x);
}

bool obj_collision(struct object& o1, struct object& o2) {
  size_t lowerY1 = o1.y+o1.height, rightX1 = o1.x+o1.width,
         lowerY2 = o2.y+o2.height, rightX2 = o2.x+o2.width;
  return ((lowerY1 <= lowerY2 && lowerY1 >= o2.y) || // bottom of o1 inside o2
          ((o1.y <= lowerY2 && o1.y >= o2.y)))    && // top of o1 inside o2
         ((rightX1 <= rightX2 && rightX1 >= o2.x) || // right side of o1 inside o2
          (o1.x <= rightX2 && o1.x >= o2.x));        // left side of o1 inside o2;
}

void enmy_fire_projectile(struct enemy& enmy) {
  enmy.proj.y = enmy.y+1;
  enmy.proj.x = enmy.x;
  enmy.proj.pan = new_panel(newwin(1, 1, enmy.proj.y, enmy.proj.x));
  wattron(panel_window(enmy.proj.pan), COLOR_PAIR(CYAN));
  waddch(panel_window(enmy.proj.pan), 'v');
  wattroff(panel_window(enmy.proj.pan), COLOR_PAIR(CYAN));
}

void enmy_move_projectile(struct enemy& enmy, int& cycle) {
  if (enmy.proj.y+1 < (size_t)LINES) {
    switch(cycle) {
    case 0: case 1:
      move_panel(enmy.proj.pan, ++enmy.proj.y, --enmy.proj.x);
      ++cycle;
      break;
    case 2:
      move_panel(enmy.proj.pan, ++enmy.proj.y, ++enmy.proj.x);
      ++cycle;
      break;
    case 3:
      move_panel(enmy.proj.pan, ++enmy.proj.y, ++enmy.proj.x);
      cycle = 0;
      break;
    default:
      cycle = 0;
    }
  }
  else
    destroy_object(enmy.proj);
  if (enmy.proj.pan && obj_collision(enmy.proj, usr)) {
    destroy_object(usr);
    destroy_object(enmy.proj);
  }
}

void enmy_flash_death(struct enemy& enmy) {
  wattron(panel_window(cpu.pan), A_BOLD);
  wclear(panel_window(cpu.pan));
  wprintw(panel_window(cpu.pan), cpu.str);
  update_panels();
  doupdate();
  std::this_thread::sleep_for (std::chrono::milliseconds(50));
  wattroff(panel_window(cpu.pan), A_BOLD);
  wattron(panel_window(cpu.pan), COLOR_PAIR(RED));
  wclear(panel_window(cpu.pan));
  wprintw(panel_window(cpu.pan), cpu.str);
  update_panels();
  doupdate();
  std::this_thread::sleep_for (std::chrono::milliseconds(50));
  wattron(panel_window(cpu.pan), A_BOLD);
  wclear(panel_window(cpu.pan));
  wprintw(panel_window(cpu.pan), cpu.str);
  update_panels();
  doupdate();
  std::this_thread::sleep_for (std::chrono::milliseconds(50));
  wattroff(panel_window(cpu.pan), A_BOLD);
  wattroff(panel_window(cpu.pan), COLOR_PAIR(RED));
  hide_panel(cpu.pan);
}

void destroy_object(struct object& o) {
  wclear(panel_window(o.pan));
  delwin(panel_window(o.pan));
  del_panel(o.pan);
  o.pan = nullptr;
}
