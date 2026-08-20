#include "TestSupport.h"

#include "MazeGenerator.h"

void runMazeTests(int& passed, int& failed) {
    MazeGenerator gen;

    Maze maze = gen.generate(MAZE_WIDTH, MAZE_HEIGHT, 1);
    CHECK(maze.width() == MAZE_WIDTH, "maze width is 64");
    CHECK(maze.height() == MAZE_HEIGHT, "maze height is 64");
    CHECK(maze.isWalkable(maze.start()), "start is floor");
    CHECK(maze.isWalkable(maze.exitPosition()), "exit is floor");
    CHECK(gen.isSolvable(maze, maze.start(), maze.exitPosition()), "generated maze is solvable");
    CHECK(maze.start() != maze.exitPosition(), "start and exit are different cells");

    Maze regenerated = gen.generate(MAZE_WIDTH, MAZE_HEIGHT, 99);
    CHECK(regenerated.isWalkable(regenerated.start()), "regenerated start is floor");
    CHECK(regenerated.isWalkable(regenerated.exitPosition()), "regenerated exit is floor");
    CHECK(gen.isSolvable(regenerated, regenerated.start(), regenerated.exitPosition()),
          "regenerated maze is solvable");

    Maze a = gen.generate(MAZE_WIDTH, MAZE_HEIGHT, 7);
    Maze b = gen.generate(MAZE_WIDTH, MAZE_HEIGHT, 8);
    CHECK(!(a == b), "different seeds produce different mazes");

    Maze aAgain = gen.generate(MAZE_WIDTH, MAZE_HEIGHT, 7);
    CHECK(a == aAgain, "same seed produces the same maze");
}
