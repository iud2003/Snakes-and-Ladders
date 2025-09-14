#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FLOORS 3
#define MAZE_WIDTH 10
#define MAZE_LENGTH 25
#define MAX_STAIRS 50
#define MAX_POLES 50
#define MAX_WALLS 200
#define MAX_PLAYERS 3
#define INITIAL_MOVEMENT_POINTS 100
#define MAX_MOVEMENT_POINTS 1000  
#define BAWANA_CELLS 16
#define STAIR_DIRECTION_CHANGE_ROUNDS 5

typedef enum {
    EMPTY = 1,
    NORTH = 2,
    EAST = 3,
    SOUTH = 4,
    WEST = 5
} Direction;

typedef enum {
    EFFECT_NONE = 0,
    EFFECT_CONSUMABLE,
    EFFECT_BONUS_ADD,
    EFFECT_BONUS_MULTIPLY
} CellEffectType;

typedef enum {
    BAWANA_FOOD_POISONING = 0,
    BAWANA_DISORIENTED,
    BAWANA_TRIGGERED,
    BAWANA_HAPPY,
    BAWANA_RANDOM_POINTS
} BawanaEffect;

typedef struct {
    CellEffectType effect_type;
    int effect_value;
    BawanaEffect bawana_effect;
} Cell;

typedef struct {
    char name;
    int floor;
    int width;
    int length;
    Direction direction;
    bool in_maze;
    int dice_throw_count;
    int movement_points;
    
    int food_poisoning_turns;
    int disoriented_turns;
    bool triggered;
    bool in_bawana;
    Direction random_direction;
} Player;

typedef struct {
    int start_floor, start_width, start_length;
    int end_floor, end_width, end_length;
    bool up_direction;
} Stair;

typedef struct {
    int start_floor, end_floor;
    int width, length;
} Pole;

typedef struct {
    int floor;
    int start_width, start_length;
    int end_width, end_length;
} Wall;

typedef struct {
    Player players[MAX_PLAYERS];
    Stair stairs[MAX_STAIRS];
    Pole poles[MAX_POLES];
    Wall walls[MAX_WALLS];
    Cell maze[MAX_FLOORS][MAZE_WIDTH][MAZE_LENGTH];
    int num_stairs, num_poles, num_walls;
    int flag_floor, flag_width, flag_length;
    bool game_over;
    char winner;
    int round_count;
} Game;

void initialize_game(Game* game);
void initialize_players(Game* game);
void initialize_maze_cells(Game* game);
void initialize_bawana_area(Game* game);
void load_stairs_from_file(Game* game, const char* filename);
void load_poles_from_file(Game* game, const char* filename);
void load_walls_from_file(Game* game, const char* filename);
void load_flag_from_file(Game* game, const char* filename);
void load_seed_from_file(const char* filename);
int roll_movement_dice();
Direction roll_direction_dice();
Direction get_random_direction();
bool is_valid_position(int floor, int width, int length);
bool is_floor_accessible(int floor, int width, int length);
bool is_in_bawana(int width, int length);
bool is_path_blocked_by_wall(Game* game, int floor, int start_w, int start_l, int end_w, int end_l);
bool can_move_single_step(Game* game, int floor, int from_w, int from_l, int to_w, int to_l);
void move_player_with_effects(Game* game, Player* player, Direction dir, int steps);
void apply_cell_effects(Game* game, Player* player, int floor, int width, int length, int* cost);
void apply_bawana_effect(Game* game, Player* player);
bool check_and_use_stairs_poles(Game* game, Player* player);
void capture_player(Game* game, int capturer_index, int captured_index);
bool is_position_occupied(Game* game, int floor, int width, int length, int exclude_player);
void change_stair_directions(Game* game);
void transport_to_bawana(Game* game, Player* player);
void print_game_state(Game* game);
void print_player_status(Player* player);
const char* direction_to_string(Direction dir);
const char* bawana_effect_to_string(BawanaEffect effect);
const char* get_cell_type_name(BawanaEffect effect);
void play_turn(Game* game, int player_index);
void play_game(Game* game);

void check_and_cap_movement_points(Game* game, Player* player) {
    if (player->movement_points > MAX_MOVEMENT_POINTS) {
        player->movement_points = MAX_MOVEMENT_POINTS;
    }
    
    if (player->movement_points <= 0) {
        transport_to_bawana(game, player);
    }
}

int main() {
    Game game;
    
    load_seed_from_file("seed.txt");
    
    initialize_game(&game);
    
    load_stairs_from_file(&game, "stairs.txt");
    load_poles_from_file(&game, "poles.txt");
    load_walls_from_file(&game, "walls.txt");
    load_flag_from_file(&game, "flag.txt");
    
    printf("=== MAZE TO SAVOR - ENHANCED UCSC MAZE RUNNER ===\n");
    printf("Game initialized from configuration files.\n\n");
    
    play_game(&game);
    
    return 0;
}

void load_seed_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Warning: Cannot open %s, using time-based seed\n", filename);
        srand(time(NULL));
        return;
    }
    
    int seed;
    if (fscanf(file, "%d", &seed) == 1) {
        srand(seed);
        printf("Random seed loaded: %d\n", seed);
    } else {
        printf("Warning: Invalid seed file, using time-based seed\n");
        srand(time(NULL));
    }
    
    fclose(file);
}

void load_stairs_from_file(Game* game, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open %s\n", filename);
        return;
    }
    
    game->num_stairs = 0;
    int start_floor, start_width, start_length, end_floor, end_width, end_length;
    
    while (fscanf(file, "%d, %d, %d, %d, %d, %d", 
                  &start_floor, &start_width, &start_length, 
                  &end_floor, &end_width, &end_length) == 6 && 
           game->num_stairs < MAX_STAIRS) {
        
        game->stairs[game->num_stairs].start_floor = start_floor;
        game->stairs[game->num_stairs].start_width = start_width;
        game->stairs[game->num_stairs].start_length = start_length;
        game->stairs[game->num_stairs].end_floor = end_floor;
        game->stairs[game->num_stairs].end_width = end_width;
        game->stairs[game->num_stairs].end_length = end_length;
        game->stairs[game->num_stairs].up_direction = true; 
        game->num_stairs++;
    }
    
    printf("Loaded %d stairs from %s\n", game->num_stairs, filename);
    fclose(file);
}

void load_poles_from_file(Game* game, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open %s\n", filename);
        return;
    }
    
    game->num_poles = 0;
    int start_floor, end_floor, width, length;
    
    while (fscanf(file, "%d, %d, %d, %d", &start_floor, &end_floor, &width, &length) == 4 && 
           game->num_poles < MAX_POLES) {
        
        game->poles[game->num_poles].start_floor = start_floor;
        game->poles[game->num_poles].end_floor = end_floor;
        game->poles[game->num_poles].width = width;
        game->poles[game->num_poles].length = length;
        game->num_poles++;
    }
    
    printf("Loaded %d poles from %s\n", game->num_poles, filename);
    fclose(file);
}

void load_walls_from_file(Game* game, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open %s\n", filename);
        return;
    }
    
    game->num_walls = 0;
    int floor, start_width, start_length, end_width, end_length;
    
    while (fscanf(file, "%d, %d, %d, %d, %d", 
                  &floor, &start_width, &start_length, &end_width, &end_length) == 5 && 
           game->num_walls < MAX_WALLS) {
        
        game->walls[game->num_walls].floor = floor;
        game->walls[game->num_walls].start_width = start_width;
        game->walls[game->num_walls].start_length = start_length;
        game->walls[game->num_walls].end_width = end_width;
        game->walls[game->num_walls].end_length = end_length;
        game->num_walls++;
    }
    
    printf("Loaded %d walls from %s\n", game->num_walls, filename);
    fclose(file);
}

void load_flag_from_file(Game* game, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open %s\n", filename);
        return;
    }
    
    if (fscanf(file, "%d, %d, %d", &game->flag_floor, &game->flag_width, &game->flag_length) == 3) {
        printf("Flag loaded at [%d, %d, %d]\n", game->flag_floor, game->flag_width, game->flag_length);
    } else {
        printf("Error: Invalid flag file format\n");
    }
    
    fclose(file);
}

void initialize_game(Game* game) {
    game->num_stairs = 0;
    game->num_poles = 0;
    game->num_walls = 0;
    game->game_over = false;
    game->winner = '\0';
    game->round_count = 0;
    
    initialize_players(game);
    initialize_maze_cells(game);
    initialize_bawana_area(game);
}

void initialize_players(Game* game) {
    game->players[0].name = 'A';
    game->players[0].floor = 0;
    game->players[0].width = 6;
    game->players[0].length = 12;
    game->players[0].direction = NORTH;
    game->players[0].in_maze = false;
    game->players[0].dice_throw_count = 0;
    game->players[0].movement_points = INITIAL_MOVEMENT_POINTS;
    game->players[0].food_poisoning_turns = 0;
    game->players[0].disoriented_turns = 0;
    game->players[0].triggered = false;
    game->players[0].in_bawana = false;
    
    game->players[1].name = 'B';
    game->players[1].floor = 0;
    game->players[1].width = 9;
    game->players[1].length = 8;
    game->players[1].direction = WEST;
    game->players[1].in_maze = false;
    game->players[1].dice_throw_count = 0;
    game->players[1].movement_points = INITIAL_MOVEMENT_POINTS;
    game->players[1].food_poisoning_turns = 0;
    game->players[1].disoriented_turns = 0;
    game->players[1].triggered = false;
    game->players[1].in_bawana = false;
    
    game->players[2].name = 'C';
    game->players[2].floor = 0;
    game->players[2].width = 9;
    game->players[2].length = 16;
    game->players[2].direction = EAST;
    game->players[2].in_maze = false;
    game->players[2].dice_throw_count = 0;
    game->players[2].movement_points = INITIAL_MOVEMENT_POINTS;
    game->players[2].food_poisoning_turns = 0;
    game->players[2].disoriented_turns = 0;
    game->players[2].triggered = false;
    game->players[2].in_bawana = false;
}

void initialize_maze_cells(Game* game) {
    for (int f = 0; f < MAX_FLOORS; f++) {
        for (int w = 0; w < MAZE_WIDTH; w++) {
            for (int l = 0; l < MAZE_LENGTH; l++) {
                Cell* cell = &game->maze[f][w][l];
                int rand_val = rand() % 100;
                
                if (rand_val < 25) {
                    cell->effect_type = EFFECT_CONSUMABLE;
                    cell->effect_value = 0;
                } else if (rand_val < 60) {
                    cell->effect_type = EFFECT_CONSUMABLE;
                    cell->effect_value = (rand() % 4) + 1;
                } else if (rand_val < 85) {
                    cell->effect_type = EFFECT_BONUS_ADD;
                    cell->effect_value = (rand() % 2) + 1;
                } else if (rand_val < 95) {
                    cell->effect_type = EFFECT_BONUS_ADD;
                    cell->effect_value = (rand() % 3) + 3;
                } else {
                    cell->effect_type = EFFECT_BONUS_MULTIPLY;
                    cell->effect_value = (rand() % 2) + 2;
                }
                
                cell->bawana_effect = BAWANA_RANDOM_POINTS;
            }
        }
    }
}

void initialize_bawana_area(Game* game) {
    BawanaEffect effects[BAWANA_CELLS];
    int effect_count = 0;
    
    // Add 2 of each type as per rules
    for (int i = 0; i < 2; i++) {
        effects[effect_count++] = BAWANA_FOOD_POISONING;
        effects[effect_count++] = BAWANA_DISORIENTED;
        effects[effect_count++] = BAWANA_TRIGGERED;
        effects[effect_count++] = BAWANA_HAPPY;
    }
    
    for (int i = 0; i < 8; i++) {
        effects[effect_count++] = BAWANA_RANDOM_POINTS;
    }
    
    for (int i = 0; i < BAWANA_CELLS; i++) {
        int j = rand() % BAWANA_CELLS;
        BawanaEffect temp = effects[i];
        effects[i] = effects[j];
        effects[j] = temp;
    }
    
    int idx = 0;
    for (int w = 6; w <= 9; w++) {
        for (int l = 20; l <= 24; l++) {
            if (w == 9 && l == 19) continue; 
            if (idx < BAWANA_CELLS) {
                game->maze[0][w][l].bawana_effect = effects[idx++];
            }
        }
    }
}

int roll_movement_dice() {
    return (rand() % 6) + 1;
}

Direction roll_direction_dice() {
    int roll = rand() % 6;
    switch(roll) {
        case 0: return EMPTY;
        case 1: return NORTH;
        case 2: return EAST;
        case 3: return SOUTH;
        case 4: return WEST;
        case 5: return EMPTY;
        default: return EMPTY;
    }
}

Direction get_random_direction() {
    Direction dirs[] = {NORTH, EAST, SOUTH, WEST};
    return dirs[rand() % 4];
}

bool is_valid_position(int floor, int width, int length) {
    return (floor >= 0 && floor < MAX_FLOORS &&
            width >= 0 && width < MAZE_WIDTH && 
            length >= 0 && length < MAZE_LENGTH);
}

bool is_floor_accessible(int floor, int width, int length) {
    if (!is_valid_position(floor, width, length)) return false;
    
    switch(floor) {
        case 0: return true;
        case 1:
            if (length >= 0 && length <= 7) return true;
            if (length >= 8 && length <= 16 && width >= 3 && width <= 6) return true;
            if (length >= 17 && length <= 24) return true;
            return false;
        case 2:
            return (length >= 8 && length <= 16);
        default:
            return false;
    }
}

bool is_in_bawana(int width, int length) {
    return (width >= 6 && width <= 9 && length >= 20 && length <= 24);
}

bool is_path_blocked_by_wall(Game* game, int floor, int start_w, int start_l, int end_w, int end_l) {
    for (int i = 0; i < game->num_walls; i++) {
        Wall* wall = &game->walls[i];
        if (wall->floor != floor) continue;
        
        if ((start_w == end_w && wall->start_width == wall->end_width && wall->start_width == start_w) ||
            (start_l == end_l && wall->start_length == wall->end_length && wall->start_length == start_l)) {
            
            int min_w = (start_w < end_w) ? start_w : end_w;
            int max_w = (start_w > end_w) ? start_w : end_w;
            int min_l = (start_l < end_l) ? start_l : end_l;
            int max_l = (start_l > end_l) ? start_l : end_l;
            
            int wall_min_w = (wall->start_width < wall->end_width) ? wall->start_width : wall->end_width;
            int wall_max_w = (wall->start_width > wall->end_width) ? wall->start_width : wall->end_width;
            int wall_min_l = (wall->start_length < wall->end_length) ? wall->start_length : wall->end_length;
            int wall_max_l = (wall->start_length > wall->end_length) ? wall->start_length : wall->end_length;
            
            if (!(max_w < wall_min_w || min_w > wall_max_w || max_l < wall_min_l || min_l > wall_max_l)) {
                return true;
            }
        }
    }
    return false;
}

bool can_move_single_step(Game* game, int floor, int from_w, int from_l, int to_w, int to_l) {
    if (!is_floor_accessible(floor, to_w, to_l)) return false;
    if (is_path_blocked_by_wall(game, floor, from_w, from_l, to_w, to_l)) return false;
    return true;
}

void apply_cell_effects(Game* game, Player* player, int floor, int width, int length, int* cost) {
    if (!is_valid_position(floor, width, length)) {
        *cost = 0;
        return;
    }
    
    Cell* cell = &game->maze[floor][width][length];
    *cost = 0;
    
    switch(cell->effect_type) {
        case EFFECT_CONSUMABLE:
            *cost = cell->effect_value;
            player->movement_points -= cell->effect_value;
            break;
            
        case EFFECT_BONUS_ADD:
            player->movement_points += cell->effect_value;
            break;
            
        case EFFECT_BONUS_MULTIPLY:
            if (player->movement_points <= 100) {
                player->movement_points *= cell->effect_value;
            } else {
                player->movement_points += (cell->effect_value * 20);
            }
            break;
            
        default:
            break;
    }
    
    check_and_cap_movement_points(game, player);
}

void apply_bawana_effect(Game* game, Player* player) {
    if (!is_in_bawana(player->width, player->length)) return;
    
    Cell* cell = &game->maze[0][player->width][player->length];
    
    printf("%c is place on a %s and effects take place.\n", player->name, get_cell_type_name(cell->bawana_effect));
    
    switch(cell->bawana_effect) {
        case BAWANA_FOOD_POISONING:
            player->food_poisoning_turns = 3;
            printf("%c eats from Bawana and have a bad case of food poisoning. Will need three rounds to recover.\n", player->name);
            break;
            
        case BAWANA_DISORIENTED:
            player->movement_points += 50;
            player->disoriented_turns = 4;
            player->floor = 0;
            player->width = 9;
            player->length = 19;
            player->direction = NORTH;
            printf("%c eats from Bawana and is disoriented and is placed at the entrance of Bawana with 50 movement points.\n", player->name);
            break;
            
        case BAWANA_TRIGGERED:
            player->movement_points += 50;
            player->triggered = true;
            player->floor = 0;
            player->width = 9;
            player->length = 19;
            player->direction = NORTH;
            printf("%c eats from Bawana and is triggered due to bad quality of food. %c is placed at the entrance of Bawana with 50 movement points.\n", player->name, player->name);
            break;
            
        case BAWANA_HAPPY:
            player->movement_points += 200;
            player->floor = 0;
            player->width = 9;
            player->length = 19;
            player->direction = NORTH;
            printf("%c eats from Bawana and is happy. %c is placed at the entrance of Bawana with 200 movement points.\n", player->name, player->name);
            break;
            
        case BAWANA_RANDOM_POINTS:
        default: {
            int bonus = (rand() % 91) + 10; 
            player->movement_points += bonus;
            printf("%c eats from Bawana and earns %d movement points and is placed at the [%d, %d, %d].\n", 
                   player->name, bonus, player->floor, player->width, player->length);
            break;
        }
    }
    
    check_and_cap_movement_points(game, player);
    player->in_bawana = false;
}

bool check_and_use_stairs_poles(Game* game, Player* player) {
    for (int i = 0; i < game->num_stairs; i++) {
        Stair* stair = &game->stairs[i];
        
        if (player->floor == stair->start_floor && 
            player->width == stair->start_width && 
            player->length == stair->start_length && 
            stair->up_direction) {
            
            printf("%c lands on [%d, %d, %d] which is a stair cell.\n", 
                   player->name, player->floor, player->width, player->length);
            
            player->floor = stair->end_floor;
            player->width = stair->end_width;
            player->length = stair->end_length;
            
            printf("%c takes the stairs and now placed at [%d, %d, %d] in floor %d.\n", 
                   player->name, player->width, player->length, player->floor, player->floor);
            return true;
        }
        
        if (player->floor == stair->end_floor && 
            player->width == stair->end_width && 
            player->length == stair->end_length && 
            !stair->up_direction) {
            
            printf("%c lands on [%d, %d, %d] which is a stair cell.\n", 
                   player->name, player->floor, player->width, player->length);
            
            player->floor = stair->start_floor;
            player->width = stair->start_width;
            player->length = stair->start_length;
            
            printf("%c takes the stairs and now placed at [%d, %d, %d] in floor %d.\n", 
                   player->name, player->width, player->length, player->floor, player->floor);
            return true;
        }
    }
    
    for (int i = 0; i < game->num_poles; i++) {
        Pole* pole = &game->poles[i];
        
        if (player->width == pole->width && player->length == pole->length) {
            if (player->floor > pole->end_floor && player->floor <= pole->start_floor) {
                printf("%c lands on [%d, %d, %d] which is a pole cell.\n", 
                       player->name, player->floor, player->width, player->length);
                
                player->floor = pole->end_floor;
                
                printf("%c slides down and now placed at [%d, %d, %d] in floor %d.\n", 
                       player->name, player->width, player->length, player->floor, player->floor);
                return true;
            }
        }
    }
    
    return false;
}

void capture_player(Game* game, int capturer_index, int captured_index) {
    Player* captured = &game->players[captured_index];
    
    captured->in_maze = false;
    captured->dice_throw_count = 0;
    captured->floor = 0;
    
    switch(captured->name) {
        case 'A': 
            captured->width = 6; 
            captured->length = 12; 
            captured->direction = NORTH;
            break;
        case 'B': 
            captured->width = 9; 
            captured->length = 8; 
            captured->direction = WEST;
            break;
        case 'C': 
            captured->width = 9; 
            captured->length = 16; 
            captured->direction = EAST;
            break;
    }
    
    captured->food_poisoning_turns = 0;
    captured->disoriented_turns = 0;
    captured->triggered = false;
    captured->in_bawana = false;
}

bool is_position_occupied(Game* game, int floor, int width, int length, int exclude_player) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (i == exclude_player) continue;
        Player* p = &game->players[i];
        if (p->in_maze && p->floor == floor && p->width == width && p->length == length) {
            return true;
        }
    }
    return false;
}

void change_stair_directions(Game* game) {
    for (int i = 0; i < game->num_stairs; i++) {
        game->stairs[i].up_direction = (rand() % 2 == 0);
    }
}

void transport_to_bawana(Game* game, Player* player) {
    printf("%c movement points are depleted and requires replenishment. Transporting to Bawana.\n", player->name);
    
    int bawana_positions[][2] = {
        {6,20}, {6,21}, {6,22}, {6,23}, {6,24},
        {7,20}, {7,21}, {7,22}, {7,23}, {7,24},
        {8,20}, {8,21}, {8,22}, {8,23}, {8,24},
        {9,20}, {9,21}, {9,22}, {9,23}, {9,24}
    };
    
    int valid_positions = sizeof(bawana_positions) / (2 * sizeof(int));
    int idx = rand() % valid_positions;
    
    player->floor = 0;
    player->width = bawana_positions[idx][0];
    player->length = bawana_positions[idx][1];
    player->in_bawana = true;
    player->movement_points = 10; 
    
    apply_bawana_effect(game, player);
}

void move_player_with_effects(Game* game, Player* player, Direction dir, int steps) {
    int effective_steps = steps;
    int total_cost = 0;
    int cells_moved = 0;
    
    if (player->triggered) {
        effective_steps *= 2;
    }
    
    for (int step = 0; step < effective_steps; step++) {
        int old_width = player->width;
        int old_length = player->length;
        int new_width = player->width;
        int new_length = player->length;
        
        switch(dir) {
            case NORTH: new_length--; break;
            case SOUTH: new_length++; break;
            case EAST:  new_width++; break;
            case WEST:  new_width--; break;
            default: return;
        }
        
        if (!can_move_single_step(game, player->floor, old_width, old_length, new_width, new_length)) {
            if (step == 0) {
                total_cost = 2; 
                player->movement_points -= 2;
                check_and_cap_movement_points(game, player);
            }
            break;
        }
        
        player->width = new_width;
        player->length = new_length;
        cells_moved++;
        
        int step_cost = 0;
        apply_cell_effects(game, player, player->floor, player->width, player->length, &step_cost);
        total_cost += step_cost;
        
        if (player->floor == game->flag_floor && 
            player->width == game->flag_width && 
            player->length == game->flag_length) {
            printf("\nGAME OVER! Player %c captured the flag at [%d, %d, %d]!\n", 
                   player->name, player->floor, player->width, player->length);
            game->game_over = true;
            game->winner = player->name;
            return;
        }
        
        if (player->movement_points <= 0) {
            break;
        }
        
        if (check_and_use_stairs_poles(game, player)) {
            if (player->floor == game->flag_floor && 
                player->width == game->flag_width && 
                player->length == game->flag_length) {
                printf("\nGAME OVER! Player %c captured the flag at [%d, %d, %d]!\n", 
                       player->name, player->floor, player->width, player->length);
                game->game_over = true;
                game->winner = player->name;
                return;
            }
            
            int remaining_steps = effective_steps - step - 1;
            if (remaining_steps > 0) {
                move_player_with_effects(game, player, dir, remaining_steps);
                return;
            }
        }
        
        if (is_in_bawana(player->width, player->length)) {
            apply_bawana_effect(game, player);
            break;
        }
    }
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Player* other = &game->players[i];
        if (other != player && other->in_maze && 
            other->floor == player->floor && 
            other->width == player->width && 
            other->length == player->length) {
            
            int capturer_idx = -1;
            for (int j = 0; j < MAX_PLAYERS; j++) {
                if (&game->players[j] == player) {
                    capturer_idx = j;
                    break;
                }
            }
            if (capturer_idx >= 0) {
                capture_player(game, capturer_idx, i);
            }
            break;
        }
    }
    
    if (cells_moved > 0 || total_cost > 0) {
        printf("%c moved %d cells that cost %d movement points and is left with %d and is moving in the %s.\n", 
               player->name, cells_moved, total_cost, player->movement_points, direction_to_string(player->direction));
    }
}

void print_game_state(Game* game) {
    printf("\n=== ROUND %d GAME STATE ===\n", game->round_count);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        print_player_status(&game->players[i]);
    }
    printf("Flag: [%d, %d, %d]\n", game->flag_floor, game->flag_width, game->flag_length);
    printf("==========================\n\n");
}

void print_player_status(Player* player) {
    printf("Player %c: ", player->name);
    if (!player->in_maze) {
        printf("Starting area [%d, %d, %d]", player->floor, player->width, player->length);
    } else {
        printf("[%d, %d, %d] facing %s", player->floor, player->width, player->length, 
               direction_to_string(player->direction));
    }
    
    printf(" - MP: %d", player->movement_points);
    
    if (player->food_poisoning_turns > 0) {
        printf(" [POISONED: %d turns]", player->food_poisoning_turns);
    }
    if (player->disoriented_turns > 0) {
        printf(" [DISORIENTED: %d turns]", player->disoriented_turns);
    }
    if (player->triggered) {
        printf(" [TRIGGERED: 2x speed]");
    }
    if (player->in_bawana) {
        printf(" [IN BAWANA]");
    }
    
    printf(" (throws: %d)\n", player->dice_throw_count);
}

const char* direction_to_string(Direction dir) {
    switch(dir) {
        case NORTH: return "North";
        case EAST:  return "East";
        case SOUTH: return "South";
        case WEST:  return "West";
        default:    return "Unknown";
    }
}

const char* bawana_effect_to_string(BawanaEffect effect) {
    switch(effect) {
        case BAWANA_FOOD_POISONING: return "Food Poisoning";
        case BAWANA_DISORIENTED: return "Disoriented";
        case BAWANA_TRIGGERED: return "Triggered";
        case BAWANA_HAPPY: return "Happy";
        case BAWANA_RANDOM_POINTS: return "Random Points";
        default: return "Unknown";
    }
}

const char* get_cell_type_name(BawanaEffect effect) {
    switch(effect) {
        case BAWANA_FOOD_POISONING: return "food poisoning cell";
        case BAWANA_DISORIENTED: return "disorientation cell";
        case BAWANA_TRIGGERED: return "triggering cell";
        case BAWANA_HAPPY: return "happy cell";
        case BAWANA_RANDOM_POINTS: return "random points cell";
        default: return "unknown cell";
    }
}

void play_turn(Game* game, int player_index) {
    Player* player = &game->players[player_index];
    
    if (player->food_poisoning_turns > 0) {
        printf("%c is still food poisoned and misses the turn.\n", player->name);
        player->food_poisoning_turns--;
        
        if (player->food_poisoning_turns == 0) {
            printf("%c is now fit to proceed from the food poisoning episode and now placed on a ", player->name);
            transport_to_bawana(game, player);
        }
        return;
    }
    
    if (player->in_bawana) {
        apply_bawana_effect(game, player);
        return;
    }
    
    if (!player->in_maze) {
        int roll = roll_movement_dice();
        
        if (roll == 6) {
            player->in_maze = true;
            player->dice_throw_count = 1;
            
            switch(player->name) {
                case 'A': player->width = 5; player->length = 12; break;
                case 'B': player->width = 9; player->length = 7; break;
                case 'C': player->width = 9; player->length = 17; break;
            }
            
            printf("%c is at the starting area and rolls 6 on the movement dice and is placed on [%d, %d, %d] of the maze.\n", 
                   player->name, player->floor, player->width, player->length);
            
            int cost = 0;
            apply_cell_effects(game, player, player->floor, player->width, player->length, &cost);
            printf("%c moved 1 cells that cost %d movement points and is left with %d and is moving in the %s.\n", 
                   player->name, cost, player->movement_points, direction_to_string(player->direction));
        } else {
            printf("%c is at the starting area and rolls %d on the movement dice cannot enter the maze.\n", 
                   player->name, roll);
            player->movement_points -= 2;
            check_and_cap_movement_points(game, player);
        }
    } else {
        int movement_roll = roll_movement_dice();
        Direction movement_dir = player->direction;
        
        player->dice_throw_count++;
        
        if (player->disoriented_turns > 0) {
            movement_dir = get_random_direction();
            player->disoriented_turns--;
            
            printf("%c rolls and %d on the movement dice and is disoriented and move in the %s and moves %d cells and is placed at the ", 
                   player->name, movement_roll, direction_to_string(movement_dir), movement_roll);
            
            move_player_with_effects(game, player, movement_dir, movement_roll);
            printf("[%d, %d, %d].\n", player->floor, player->width, player->length);
            
            if (player->disoriented_turns == 0) {
                printf("%c has recovered from disorientation.\n", player->name);
            }
        } else {
            if (player->dice_throw_count % 4 == 0) {
                Direction dir_roll = roll_direction_dice();
                
                if (dir_roll != EMPTY) {
                    player->direction = dir_roll;
                    movement_dir = dir_roll;
                    
                    printf("%c rolls and %d on the movement dice and %s on the direction dice, changes direction to %s and moves %d cells and is now at ", 
                           player->name, movement_roll, direction_to_string(dir_roll), 
                           direction_to_string(movement_dir), movement_roll);
                } else {
                    printf("%c rolls and %d on the movement dice and Empty on the direction dice, changes direction to %s and moves %d cells and is now at ", 
                           player->name, movement_roll, direction_to_string(movement_dir), movement_roll);
                }
            } else {
                if (player->triggered) {
                    printf("%c is triggered and rolls and %d on the movement dice and move in the %s and moves %d cells and is placed at the ", 
                           player->name, movement_roll, direction_to_string(movement_dir), movement_roll * 2);
                } else {
                    printf("%c rolls and %d on the movement dice and moves %s by %d cells and is now at ", 
                           player->name, movement_roll, direction_to_string(movement_dir), movement_roll);
                }
            }
            
            int new_width = player->width;
            int new_length = player->length;
            
            switch(movement_dir) {
                case NORTH: new_length--; break;
                case SOUTH: new_length++; break;
                case EAST:  new_width++; break;
                case WEST:  new_width--; break;
                default: break;
            }
            
            if (!can_move_single_step(game, player->floor, player->width, player->length, new_width, new_length)) {
                printf("%c rolls and %d on the movement dice and cannot move in the %s. Player remains at [%d, %d, %d]\n", 
                       player->name, movement_roll, direction_to_string(movement_dir), 
                       player->floor, player->width, player->length);
                
                player->movement_points -= 2;
                printf("%c moved 0 cells that cost 2 movement points and is left with %d and is moving in the %s.\n", 
                       player->name, player->movement_points, direction_to_string(player->direction));
                
                check_and_cap_movement_points(game, player);
            } else {
                move_player_with_effects(game, player, movement_dir, movement_roll);
                printf("[%d, %d, %d].\n", player->floor, player->width, player->length);
            }
        }
        
        if (player->triggered && player->dice_throw_count % 4 == 0) {
            player->triggered = false;
        }
    }
}

void play_game(Game* game) {
    while (!game->game_over) {
        game->round_count++;
        
        if (game->round_count % STAIR_DIRECTION_CHANGE_ROUNDS == 0) {
            change_stair_directions(game);
            printf("--- Stair directions changed at round %d ---\n", game->round_count);
        }
        
        for (int i = 0; i < MAX_PLAYERS && !game->game_over; i++) {
            printf("\n--- Player %c's Turn (Round %d) ---\n", game->players[i].name, game->round_count);
            play_turn(game, i);
            
            if (game->game_over) break;
        }
        
        if (!game->game_over && game->round_count % 10 == 0) {
            print_game_state(game);
        }
    }
    
    if (game->winner != '\0') {
        printf("\nCONGRATULATIONS PLAYER %c! YOU WON THE GAME!\n", game->winner);
    }
    
    print_game_state(game);
}