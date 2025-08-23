#include <stdio.h>

// ==== Structs ====
typedef struct {
    int floor;
    int width;
    int length;
} Cell;

typedef struct {
    int floorNumber;
    int width;
    int length;
    int totalBlocks;
    int totalArea;
} Floor;

typedef struct {
    Cell start;
    Cell end;
    int bidirectional; // 1 = both ways, 0 = only up
} Stair;

typedef struct {
    int startFloor;
    int endFloor;
    int width;
    int length;
} Pole;

typedef struct {
    char name;
    Cell position;
} Player;

// ==== Functions ====
void initFloors(Floor floors[]) {
    // Floor 1
    floors[0].floorNumber = 0;
    floors[0].width = 10;
    floors[0].length = 25;
    floors[0].totalBlocks = floors[0].width * floors[0].length; // 250
    floors[0].totalArea = 856; // given in problem

    // Floor 2
    floors[1].floorNumber = 1;
    floors[1].width = 10;
    floors[1].length = 25; // reference grid
    floors[1].totalBlocks = 10 * 25; // max cells
    floors[1].totalArea = 748;

    // Floor 3
    floors[2].floorNumber = 2;
    floors[2].width = 10;
    floors[2].length = 9;
    floors[2].totalBlocks = floors[2].width * floors[2].length; // 90
    floors[2].totalArea = 360;
}

void printCell(Cell c) {
    printf("[F%d, W%d, L%d]", c.floor, c.width, c.length);
}

Cell checkStair(Cell player, Stair stair) {
    if (player.floor == stair.start.floor &&
        player.width == stair.start.width &&
        player.length == stair.start.length) {
        printf("  -> Player stepped on stair start, moving to stair end.\n");
        return stair.end;
    }
    if (stair.bidirectional &&
        player.floor == stair.end.floor &&
        player.width == stair.end.width &&
        player.length == stair.end.length) {
        printf("  -> Player stepped on stair end, moving to stair start.\n");
        return stair.start;
    }
    return player;
}

Cell checkPole(Cell player, Pole pole) {
    if (player.width == pole.width && player.length == pole.length) {
        if (player.floor > pole.endFloor) {
            printf("  -> Player slid down the pole.\n");
            Cell newPos = {pole.endFloor, pole.width, pole.length};
            return newPos;
        }
    }
    return player;
}

// ==== Main Demo ====
int main() {
    Floor floors[3];
    initFloors(floors);

    printf("=== Maze of UCSC ===\n");
    for (int i = 0; i < 3; i++) {
        printf("Floor %d -> Width=%d, Length=%d, Blocks=%d, Area=%d sq ft\n",
               floors[i].floorNumber,
               floors[i].width, floors[i].length,
               floors[i].totalBlocks, floors[i].totalArea);
    }

    // Example stair: [0, 4, 5, 2, 0, 10]
    Stair stair1 = {{0, 4, 5}, {2, 0, 10}, 1};

    // Example pole: [0, 2, 5, 24]
    Pole pole1 = {0, 2, 5, 24}; // from floor 3 -> floor 1

    // Player at stair start
    Player p1 = {'A', {0, 4, 5}};
    printf("\nPlayer %c starts at ", p1.name);
    printCell(p1.position);
    printf("\n");

    // Check stair
    p1.position = checkStair(p1.position, stair1);
    printf("Player %c now at ", p1.name);
    printCell(p1.position);
    printf("\n");

    // Move player to pole top
    p1.position = (Cell){2, 5, 24};
    printf("\nPlayer %c moved to ", p1.name);
    printCell(p1.position);
    printf("\n");

    // Check pole
    p1.position = checkPole(p1.position, pole1);
    printf("Player %c now at ", p1.name);
    printCell(p1.position);
    printf("\n");

    return 0;
}
