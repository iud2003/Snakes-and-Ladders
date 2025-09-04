#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

// Game constants
#define MAX_FLOORS 3
#define MAZE_WIDTH 10
#define MAZE_LENGTH 25
#define MAX_STAIRS 20
#define MAX_POLES 20
#define MAX_WALLS 100
#define MAX_PLAYERS 3
#define INITIAL_MOVEMENT_POINTS 100
#define BAWANA_CELLS 16
#define STAIR_DIRECTION_CHANGE_ROUNDS 5

// Direction enumeration
typedef enum {
    EMPTY = 1,
    NORTH = 2,
    EAST = 3,
    SOUTH = 4,
    WEST = 5
} Direction;

// Cell effect types
typedef enum {
    EFFECT_NONE = 0,
    EFFECT_CONSUMABLE,
    EFFECT_BONUS_ADD,
    EFFECT_BONUS_MULTIPLY
} CellEffectType;

// Bawana effect types
typedef enum {
    BAWANA_FOOD_POISONING = 0,
    BAWANA_DISORIENTED,
    BAWANA_TRIGGERED,
    BAWANA_HAPPY,
    BAWANA_RANDOM_POINTS
} BawanaEffect;

// Cell structure
typedef struct {
    CellEffectType effect_type;
    int effect_value;
    BawanaEffect bawana_effect;
} Cell;

// Player structure
typedef struct {
    char name;
    int floor;
    int width;
    int length;
    Direction direction;
    bool in_maze;
    int dice_throw_count;
    int movement_points;
    
    // Special effects
    int food_poisoning_turns;
    int disoriented_turns;
    bool triggered;
    bool in_bawana;
    Direction random_direction;
} Player;

// Stair structure
typedef struct {
    int start_floor, start_width, start_length;
    int end_floor, end_width, end_length;
    bool up_direction; // true = start->end, false = end->start
} Stair;

// Pole structure
typedef struct {
    int start_floor, end_floor;
    int width, length;
} Pole;

// Wall structure
typedef struct {
    int floor;
    int start_width, start_length;
    int end_width, end_length;
} Wall;

// Game structure
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

// Function prototypes
void initialize_game(Game* game);
void initialize_players(Game* game);
void initialize_maze_cells(Game* game);
void initialize_bawana_area(Game* game);
void add_sample_obstacles(Game* game);
int roll_movement_dice();
Direction roll_direction_dice();
Direction get_random_direction();
bool is_valid_position(int floor, int width, int length);
bool is_floor_accessible(int floor, int width, int length);
bool is_in_bawana(int width, int length);
bool is_path_blocked_by_wall(Game* game, int floor, int start_w, int start_l, int end_w, int end_l);
bool can_move_single_step(Game* game, int floor, int from_w, int from_l, int to_w, int to_l);
void move_player_with_effects(Game* game, Player* player, Direction dir, int steps);
void apply_cell_effects(Game* game, Player* player, int floor, int width, int length);
void apply_bawana_effect(Game* game, Player* player);
void check_stairs_and_poles_during_movement(Game* game, Player* player, int* remaining_steps, Direction dir);
void capture_player(Game* game, int capturer_index, int captured_index);
bool is_position_occupied(Game* game, int floor, int width, int length, int exclude_player);
void change_stair_directions(Game* game);
void transport_to_bawana(Game* game, Player* player);
void print_game_state(Game* game);
void print_player_status(Player* player);
const char* direction_to_string(Direction dir);
const char* bawana_effect_to_string(BawanaEffect effect);
void play_turn(Game* game, int player_index);
void play_game(Game* game);

int main() {
    srand(time(NULL));
    
    Game game;
    initialize_game(&game);
    
    printf("=== MAZE TO SAVOR - ENHANCED UCSC MAZE RUNNER ===\n");
    printf("Welcome to the advanced three-floor maze adventure!\n");
    printf("Players A, B, and C compete with movement points and special effects.\n\n");
    
    printf("Enhanced Features:\n");
    printf("- Movement Points System (start with 100 points)\n");
    printf("- Cell consumption and bonus effects\n");
    printf("- Player capture mechanics\n");
    printf("- Bawana special area with food effects\n");
    printf("- Dynamic stair directions (change every 5 rounds)\n");
    printf("- Enhanced movement through stairs/poles\n\n");
    
    printf("Flag location: Floor %d, Position [%d, %d]\n\n", 
           game.flag_floor, game.flag_width, game.flag_length);
    
    play_game(&game);
    
    return 0;
}

void initialize_game(Game* game) {
    game->num_stairs = 0;
    game->num_poles = 0;
    game->num_walls = 0;
    game->game_over = false;
    game->winner = '\0';
    game->round_count = 0;
    
    // Set flag location (avoid Bawana area)
    do {
        game->flag_floor = rand() % 3;
        if (game->flag_floor == 0) {
            game->flag_width = rand() % 10;
            game->flag_length = rand() % 25;
        } else if (game->flag_floor == 1) {
            if (rand() % 2 == 0) {
                game->flag_width = rand() % 10;
                game->flag_length = rand() % 8;
            } else {
                game->flag_width = rand() % 10;
                game->flag_length = 17 + (rand() % 8);
            }
        } else {
            game->flag_width = rand() % 10;
            game->flag_length = 8 + (rand() % 9);
        }
    } while (game->flag_floor == 0 && is_in_bawana(game->flag_width, game->flag_length));
    
    initialize_players(game);
    initialize_maze_cells(game);
    initialize_bawana_area(game);
    add_sample_obstacles(game);
}

void initialize_players(Game* game) {
    // Player A
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
    
    // Player B
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
    
    // Player C
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
    // Initialize all cells with random effects based on distribution
    for (int f = 0; f < MAX_FLOORS; f++) {
        for (int w = 0; w < MAZE_WIDTH; w++) {
            for (int l = 0; l < MAZE_LENGTH; l++) {
                Cell* cell = &game->maze[f][w][l];
                int rand_val = rand() % 100;
                
                if (rand_val < 25) {
                    // 25% - Zero consumable value
                    cell->effect_type = EFFECT_CONSUMABLE;
                    cell->effect_value = 0;
                } else if (rand_val < 60) {
                    // 35% - Consumable value 1-4
                    cell->effect_type = EFFECT_CONSUMABLE;
                    cell->effect_value = (rand() % 4) + 1;
                } else if (rand_val < 85) {
                    // 25% - Bonus 1-2
                    cell->effect_type = EFFECT_BONUS_ADD;
                    cell->effect_value = (rand() % 2) + 1;
                } else if (rand_val < 95) {
                    // 10% - Bonus 3-5
                    cell->effect_type = EFFECT_BONUS_ADD;
                    cell->effect_value = (rand() % 3) + 3;
                } else {
                    // 5% - Multiply by 2 or 3
                    cell->effect_type = EFFECT_BONUS_MULTIPLY;
                    cell->effect_value = (rand() % 2) + 2;
                }
                
                cell->bawana_effect = BAWANA_RANDOM_POINTS;
            }
        }
    }
}

void initialize_bawana_area(Game* game) {
    // Bawana area: from [6,20] to [9,24] on ground floor
    BawanaEffect effects[BAWANA_CELLS];
    int effect_count = 0;
    
    // 3 of each special effect
    for (int i = 0; i < 3; i++) {
        effects[effect_count++] = BAWANA_FOOD_POISONING;
        effects[effect_count++] = BAWANA_DISORIENTED;
        effects[effect_count++] = BAWANA_TRIGGERED;
        effects[effect_count++] = BAWANA_HAPPY;
    }
    
    // 4 random movement point cells
    for (int i = 0; i < 4; i++) {
        effects[effect_count++] = BAWANA_RANDOM_POINTS;
    }
    
    // Shuffle effects
    for (int i = 0; i < BAWANA_CELLS; i++) {
        int j = rand() % BAWANA_CELLS;
        BawanaEffect temp = effects[i];
        effects[i] = effects[j];
        effects[j] = temp;
    }
    
    // Assign effects to Bawana cells
    int idx = 0;
    for (int w = 6; w <= 9; w++) {
        for (int l = 20; l <= 24; l++) {
            if (w == 9 && l == 19) continue; // Skip entrance
            game->maze[0][w][l].bawana_effect = effects[idx++];
        }
    }
}

void add_sample_obstacles(Game* game) {
    // Add stairs with initial directions
    game->stairs[0] = (Stair){0, 2, 5, 1, 2, 5, true};
    game->stairs[1] = (Stair){1, 5, 10, 2, 5, 10, true};
    game->stairs[2] = (Stair){0, 7, 15, 2, 7, 15, true};
    game->num_stairs = 3;
    
    // Add poles
    game->poles[0] = (Pole){2, 0, 7, 12};
    game->poles[1] = (Pole){1, 0, 3, 18};
    game->num_poles = 2;
    
    // Add walls (including Bawana walls)
    game->walls[0] = (Wall){0, 0, 10, 9, 10};
    game->walls[1] = (Wall){0, 5, 0, 5, 5};
    game->walls[2] = (Wall){1, 2, 5, 7, 5};
    // Bawana walls
    game->walls[3] = (Wall){0, 6, 20, 9, 20}; // North wall
    game->walls[4] = (Wall){0, 6, 20, 6, 24}; // West wall
    game->num_walls = 5;
}

int roll_movement_dice() {
    return (rand() % 6) + 1;
}

Direction roll_direction_dice() {
    int roll = rand() % 6;
    switch(roll) {
        case 1: return EMPTY;   // Face 1 -> Empty
        case 2: return NORTH;   // Face 2 -> North
        case 3: return EAST;    // Face 3 -> East
        case 4: return SOUTH;   // Face 4 -> South
        case 5: return WEST;    // Face 5 -> West
        case 6: return EMPTY;   // Face 6 -> Empty
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
        
        // Simplified wall collision check
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

void apply_cell_effects(Game* game, Player* player, int floor, int width, int length) {
    if (!is_valid_position(floor, width, length)) return;
    
    Cell* cell = &game->maze[floor][width][length];
    
    switch(cell->effect_type) {
        case EFFECT_CONSUMABLE:
            player->movement_points -= cell->effect_value;
            if (cell->effect_value > 0) {
                printf("Cell consumed %d movement points from Player %c (now: %d)\n", 
                       cell->effect_value, player->name, player->movement_points);
            }
            break;
            
        case EFFECT_BONUS_ADD:
            player->movement_points += cell->effect_value;
            printf("Cell gave Player %c bonus +%d movement points (now: %d)\n", 
                   player->name, cell->effect_value, player->movement_points);
            break;
            
        case EFFECT_BONUS_MULTIPLY:
            player->movement_points *= cell->effect_value;
            printf("Cell multiplied Player %c movement points by %d (now: %d)\n", 
                   player->name, cell->effect_value, player->movement_points);
            break;
            
        default:
            break;
    }
    
    // Check if movement points fell to zero or negative
    if (player->movement_points <= 0) {
        printf("Player %c ran out of movement points! Transported to Bawana!\n", player->name);
        transport_to_bawana(game, player);
    }
}

void apply_bawana_effect(Game* game, Player* player) {
    if (!is_in_bawana(player->width, player->length)) return;
    
    Cell* cell = &game->maze[0][player->width][player->length];
    
    switch(cell->bawana_effect) {
        case BAWANA_FOOD_POISONING:
            player->food_poisoning_turns = 3;
            printf("Player %c got food poisoning! Missing next 3 turns!\n", player->name);
            break;
            
        case BAWANA_DISORIENTED:
            player->movement_points += 50;
            player->disoriented_turns = 4;
            player->floor = 0;
            player->width = 9;
            player->length = 19;
            player->direction = NORTH;
            printf("Player %c is disoriented! +50 points, moved to entrance, random movement for 4 turns!\n", player->name);
            break;
            
        case BAWANA_TRIGGERED:
            player->movement_points += 50;
            player->triggered = true;
            player->floor = 0;
            player->width = 9;
            player->length = 19;
            player->direction = NORTH;
            printf("Player %c is triggered! +50 points, moved to entrance, moves twice as fast!\n", player->name);
            break;
            
        case BAWANA_HAPPY:
            player->movement_points += 200;
            player->floor = 0;
            player->width = 9;
            player->length = 19;
            player->direction = NORTH;
            printf("Player %c is happy! +200 points, moved to entrance!\n", player->name);
            break;
            
        case BAWANA_RANDOM_POINTS:
        default: {
            int bonus = (rand() % 91) + 10; // 10-100 points
            player->movement_points += bonus;
            printf("Player %c got %d random movement points in Bawana!\n", player->name, bonus);
            break;
        }
    }
    
    player->in_bawana = false;
}

void check_stairs_and_poles_during_movement(Game* game, Player* player, int* remaining_steps, Direction dir) {
    // Check stairs
    for (int i = 0; i < game->num_stairs; i++) {
        Stair* stair = &game->stairs[i];
        
        // Check if player stepped on stair
        bool on_stair_start = (player->floor == stair->start_floor && 
                              player->width == stair->start_width && 
                              player->length == stair->start_length);
        bool on_stair_end = (player->floor == stair->end_floor && 
                            player->width == stair->end_width && 
                            player->length == stair->end_length);
        
        if (on_stair_start && stair->up_direction) {
            player->floor = stair->end_floor;
            player->width = stair->end_width;
            player->length = stair->end_length;
            printf("Player %c used stairs up! Now at [%d, %d, %d], %d steps remaining\n", 
                   player->name, player->floor, player->width, player->length, *remaining_steps);
            return;
        } else if (on_stair_end && !stair->up_direction) {
            player->floor = stair->start_floor;
            player->width = stair->start_width;
            player->length = stair->start_length;
            printf("Player %c used stairs down! Now at [%d, %d, %d], %d steps remaining\n", 
                   player->name, player->floor, player->width, player->length, *remaining_steps);
            return;
        }
    }
    
    // Check poles
    for (int i = 0; i < game->num_poles; i++) {
        Pole* pole = &game->poles[i];
        
        if (player->width == pole->width && player->length == pole->length) {
            if (player->floor >= pole->start_floor && player->floor > pole->end_floor) {
                player->floor = pole->end_floor;
                printf("Player %c slid down pole! Now at [%d, %d, %d], %d steps remaining\n", 
                       player->name, player->floor, player->width, player->length, *remaining_steps);
                return;
            }
        }
    }
}

void capture_player(Game* game, int capturer_index, int captured_index) {
    Player* captured = &game->players[captured_index];
    
    printf("Player %c captured Player %c! Sending to starting area.\n", 
           game->players[capturer_index].name, captured->name);
    
    // Move captured player to starting area
    captured->in_maze = false;
    captured->dice_throw_count = 0;
    switch(captured->name) {
        case 'A': captured->width = 6; captured->length = 12; break;
        case 'B': captured->width = 9; captured->length = 8; break;
        case 'C': captured->width = 9; captured->length = 16; break;
    }
    captured->floor = 0;
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
    printf("\n*** STAIR DIRECTIONS CHANGING! ***\n");
    for (int i = 0; i < game->num_stairs; i++) {
        game->stairs[i].up_direction = (rand() % 2 == 0);
        printf("Stair %d now goes %s\n", i+1, 
               game->stairs[i].up_direction ? "UP" : "DOWN");
    }
    printf("*** END STAIR DIRECTION CHANGE ***\n\n");
}

void transport_to_bawana(Game* game, Player* player) {
    // Randomly place in Bawana area
    int bawana_positions[][2] = {
        {6,20}, {6,21}, {6,22}, {6,23}, {6,24},
        {7,20}, {7,21}, {7,22}, {7,23}, {7,24},
        {8,20}, {8,21}, {8,22}, {8,23}, {8,24},
        {9,20}, {9,21}, {9,22}, {9,23}, {9,24}
    };
    
    int idx = rand() % BAWANA_CELLS;
    player->floor = 0;
    player->width = bawana_positions[idx][0];
    player->length = bawana_positions[idx][1];
    player->in_bawana = true;
    player->movement_points = 1; // Give 1 point to prevent immediate re-transport
    
    printf("Player %c transported to Bawana at [0, %d, %d]!\n", 
           player->name, player->width, player->length);
}

void move_player_with_effects(Game* game, Player* player, Direction dir, int steps) {
    int effective_steps = steps;
    
    // Apply triggered effect (double movement)
    if (player->triggered) {
        effective_steps *= 2;
        printf("Player %c is triggered! Moving %d steps instead of %d\n", 
               player->name, effective_steps, steps);
    }
    
    // Move step by step to handle stairs/poles and cell effects
    for (int step = 0; step < effective_steps; step++) {
        int new_width = player->width;
        int new_length = player->length;
        
        // Calculate next position
        switch(dir) {
            case NORTH: new_length--; break;
            case SOUTH: new_length++; break;
            case EAST:  new_width++; break;
            case WEST:  new_width--; break;
            default: return;
        }
        
        // Check if move is possible
        if (!can_move_single_step(game, player->floor, player->width, player->length, new_width, new_length)) {
            printf("Player %c blocked after %d steps\n", player->name, step);
            player->movement_points -= 2; // Penalty for blocked movement
            break;
        }
        
        // Check for player capture
        int occupied_by = -1;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (game->players[i].in_maze && 
                game->players[i].floor == player->floor && 
                game->players[i].width == new_width && 
                game->players[i].length == new_length) {
                occupied_by = i;
                break;
            }
        }
        
        // Move player
        player->width = new_width;
        player->length = new_length;
        
        // Handle player capture
        if (occupied_by >= 0) {
            int capturer_idx = -1;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (&game->players[i] == player) {
                    capturer_idx = i;
                    break;
                }
            }
            if (capturer_idx >= 0) {
                capture_player(game, capturer_idx, occupied_by);
            }
        }
        
        // Apply cell effects
        apply_cell_effects(game, player, player->floor, player->width, player->length);
        
        if (player->movement_points <= 0) break;
        
        // Check for stairs and poles during movement
        int remaining = effective_steps - step - 1;
        check_stairs_and_poles_during_movement(game, player, &remaining, dir);
        
        // Check if in Bawana
        if (is_in_bawana(player->width, player->length)) {
            apply_bawana_effect(game, player);
            break;
        }
    }
    
    printf("Player %c moved to [%d, %d, %d] with %d movement points\n", 
           player->name, player->floor, player->width, player->length, player->movement_points);
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void play_turn(Game* game, int player_index) {
    Player* player = &game->players[player_index];
    
    printf("\n--- Player %c's Turn ---\n", player->name);
    
    // Check for food poisoning
    if (player->food_poisoning_turns > 0) {
        printf("Player %c is food poisoned! Skipping turn (%d turns remaining)\n", 
               player->name, player->food_poisoning_turns);
        player->food_poisoning_turns--;
        
        if (player->food_poisoning_turns == 0) {
            printf("Player %c recovered from food poisoning! Randomly placed in Bawana.\n", player->name);
            transport_to_bawana(game, player);
            apply_bawana_effect(game, player);
        }
        return;
    }
    
    // Handle Bawana effects
    if (player->in_bawana) {
        apply_bawana_effect(game, player);
        if (player->in_bawana) return; // Still in Bawana after effect
    }
    
    if (!player->in_maze) {
        // Player needs to roll 6 to enter maze
        int roll = roll_movement_dice();
        printf("Player %c rolled %d to enter maze\n", player->name, roll);
        
        if (roll == 6) {
            player->in_maze = true;
            player->dice_throw_count = 1;
            
            // Move to first maze cell
            switch(player->name) {
                case 'A': player->width = 5; player->length = 12; break;
                case 'B': player->width = 9; player->length = 7; break;
                case 'C': player->width = 9; player->length = 17; break;
            }
            
            printf("Player %c entered the maze at [%d, %d, %d]!\n", 
                   player->name, player->floor, player->width, player->length);
            
            // Apply cell effects immediately
            apply_cell_effects(game, player, player->floor, player->width, player->length);
        } else {
            printf("Player %c stays in starting area\n", player->name);
            player->movement_points -= 2; // Cost for failed entry attempt
        }
    } else {
        // Player is in maze
        int movement_roll = roll_movement_dice();
        Direction movement_dir = player->direction;
        
        player->dice_throw_count++;
        
        // Handle disorientation
        if (player->disoriented_turns > 0) {
            movement_dir = get_random_direction();
            player->disoriented_turns--;
            printf("Player %c is disoriented! Moving randomly %s for %d steps (%d turns left)\n", 
                   player->name, direction_to_string(movement_dir), movement_roll, player->disoriented_turns);
        } else {
            // Roll direction dice every 4th throw
            if (player->dice_throw_count % 4 == 0) {
                Direction dir_roll = roll_direction_dice();
                printf("Player %c rolled direction dice %d  , Player changed the direction into %s \n",player->name, dir_roll,(dir_roll == EMPTY) ? "Empty (keep current)" : direction_to_string(dir_roll));
                printf("Player %c rolled movement: %d,\n ", 
                       player->name, movement_roll);
                
                if (dir_roll != EMPTY) {
                    player->direction = dir_roll;
                    movement_dir = dir_roll;
                }
            } else {
                printf("Player %c rolled movement: %d (continuing %s)\n", 
                       player->name, movement_roll, direction_to_string(movement_dir));
            }
        }
        
        // Attempt to move with enhanced effects
        move_player_with_effects(game, player, movement_dir, movement_roll);
        
        // Check if player captured the flag
        if (player->floor == game->flag_floor && 
            player->width == game->flag_width && 
            player->length == game->flag_length) {
            printf("\n🎉 GAME OVER! Player %c captured the flag! 🎉\n", player->name);
            game->game_over = true;
            game->winner = player->name;
        }
        
        // Reset triggered effect after one use
        if (player->triggered && player->dice_throw_count % 4 == 0) {
            player->triggered = false;
            printf("Player %c is no longer triggered\n", player->name);
        }
    }
    
    print_player_status(player);
}
/////////////////////////////////////////////////////////////////////////////////////////////

void play_game(Game* game) {
    while (!game->game_over) {
        game->round_count++;
        printf("\n========== ROUND %d ==========\n", game->round_count);
        
        // Change stair directions every 5 rounds
        if (game->round_count % STAIR_DIRECTION_CHANGE_ROUNDS == 0) {
            change_stair_directions(game);
        }
        
        // Each player takes a turn
        for (int i = 0; i < MAX_PLAYERS && !game->game_over; i++) {
            play_turn(game, i);
            
            if (game->game_over) break;
            
            
        }
        
        if (!game->game_over) {
            print_game_state(game);
            
            // Safety check to prevent infinite games
            if (game->round_count <0) {
                printf("Game reached maximum rounds. Ending game.\n");
                break;
            }
        }
    }
    
    if (game->winner != '\0') {
        printf("\n🏆 CONGRATULATIONS PLAYER %c! 🏆\n", game->winner);
        printf("You successfully navigated the enhanced maze and captured the flag!\n");
        printf("Final Statistics:\n");
        for (int i = 0; i < MAX_PLAYERS; i++) {
            printf("Player %c: %d movement points, %d dice throws\n", 
                   game->players[i].name, game->players[i].movement_points, 
                   game->players[i].dice_throw_count);
        }
    }
    
    print_game_state(game);
}
