void runMazeTests(int& passed, int& failed);
void runEventTests(int& passed, int& failed);
void runGodTests(int& passed, int& failed);
void runCombatTests(int& passed, int& failed);

#include <iostream>

int main() {
    int passed = 0;
    int failed = 0;
    runMazeTests(passed, failed);
    runEventTests(passed, failed);
    runGodTests(passed, failed);
    runCombatTests(passed, failed);
    std::cout << "Passed: " << passed << "  Failed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}
