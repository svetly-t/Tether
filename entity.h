#include <vector>

class Game {
    public:
        void Update();
        void Draw();
    private:
        std::vector<Entity> entities;
};

class Entity {
    public:
        int x;
        int y;
};