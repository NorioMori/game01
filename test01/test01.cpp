// Console Roguelike: Dungeon of the Lost Stars
// Controls: W/A/S/D move, Q quit, P potion

#include <algorithm>
#include <cmath>
#include <conio.h>
#include <iostream>
#include <random>
#include <string>
#include <vector>

struct Position
{
	int x;
	int y;

	bool operator==(const Position& other) const
	{
		return x == other.x && y == other.y;
	}
};

struct Enemy
{
	Position position;
	int health = 2;
	char symbol = 'g';
};

class Game
{
public:
	Game() : random(std::random_device{}())
	{
		generateDungeon();
	}

	void run()
	{
		while (!gameOver)
		{
			draw();
			const char command = static_cast<char>(_getch());

			if (command == 'q' || command == 'Q')
			{
				gameOver = true;
				break;
			}

			if (command == 'p' || command == 'P')
			{
				usePotion();
				continue;
			}

			if (movePlayer(command))
			{
				moveEnemies();
				checkVictory();
			}
		}

		draw();
		std::cout << "\n" << (won ? "*** YOU ESCAPED THE DUNGEON! ***" : "The dungeon claims another adventurer...") << "\n";
		std::cout << "Final score: " << score << "   Press any key to exit.\n";
		_getch();
	}

private:
	static constexpr int width = 48;
	static constexpr int height = 22;
	static constexpr int relicGoal = 3;

	std::vector<std::string> map;
	std::vector<Enemy> enemies;
	Position player{};
	Position exitPosition{};
	std::mt19937 random;
	int health = 12;
	int potions = 2;
	int relics = 0;
	int score = 0;
	int turns = 0;
	bool gameOver = false;
	bool won = false;

	int randomInt(int minimum, int maximum)
	{
		return std::uniform_int_distribution<int>(minimum, maximum)(random);
	}

	bool isFloor(const Position& position) const
	{
		return position.x > 0 && position.x < width - 1 &&
			   position.y > 0 && position.y < height - 1 &&
			   map[position.y][position.x] != '#';
	}

	bool occupied(const Position& position) const
	{
		if (position == player || position == exitPosition)
			return true;

		return std::any_of(enemies.begin(), enemies.end(),
			[&position](const Enemy& enemy) { return enemy.position == position; });
	}

	Position randomEmptyPosition()
	{
		Position position{};
		do
		{
			position = { randomInt(1, width - 2), randomInt(1, height - 2) };
		} while (!isFloor(position) || occupied(position));
		return position;
	}

	void generateDungeon()
	{
		map.assign(height, std::string(width, '.'));
		for (int y = 0; y < height; ++y)
		{
			map[y][0] = '#';
			map[y][width - 1] = '#';
		}
		for (int x = 0; x < width; ++x)
		{
			map[0][x] = '#';
			map[height - 1][x] = '#';
		}

		for (int i = 0; i < 70; ++i)
		{
			const Position wall{ randomInt(2, width - 3), randomInt(2, height - 3) };
			map[wall.y][wall.x] = '#';
		}

		player = { 2, 2 };
		map[player.y][player.x] = '.';
		exitPosition = randomEmptyPosition();

		for (int i = 0; i < relicGoal; ++i)
		{
			const Position relic = randomEmptyPosition();
			map[relic.y][relic.x] = 'R';
		}

		for (int i = 0; i < 8; ++i)
		{
			Enemy enemy;
			enemy.position = randomEmptyPosition();
			enemy.symbol = (i % 3 == 0) ? 'O' : 'g';
			enemy.health = enemy.symbol == 'O' ? 3 : 2;
			enemies.push_back(enemy);
		}
	}

	void draw() const
	{
		std::cout << "\x1b[2J\x1b[H";
		std::cout << "DUNGEON OF THE LOST STARS | WASD: move  P: potion  Q: quit\n";
		std::cout << "HP: " << health << "/12  Potions: " << potions
				  << "  Relics: " << relics << "/" << relicGoal
				  << "  Score: " << score << "  Turns: " << turns << "\n\n";

		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const Position current{ x, y };
				char tile = map[y][x];
				if (current == player)
					tile = '@';
				else if (current == exitPosition)
					tile = relics == relicGoal ? '>' : 'X';
				else
				{
					for (const Enemy& enemy : enemies)
					{
						if (enemy.position == current)
						{
							tile = enemy.symbol;
							break;
						}
					}
				}
				std::cout << tile;
			}
			std::cout << '\n';
		}
		std::cout << "\n@: you  R: star relic  >: exit  X: locked exit  g/O: monsters\n";
	}

	bool movePlayer(char command)
	{
		Position direction{};
		switch (command)
		{
		case 'w': case 'W': direction = { 0, -1 }; break;
		case 's': case 'S': direction = { 0, 1 }; break;
		case 'a': case 'A': direction = { -1, 0 }; break;
		case 'd': case 'D': direction = { 1, 0 }; break;
		default: return false;
		}

		const Position target{ player.x + direction.x, player.y + direction.y };
		if (!isFloor(target) && map[target.y][target.x] != 'R')
			return false;

		const auto enemy = std::find_if(enemies.begin(), enemies.end(),
			[&target](const Enemy& candidate) { return candidate.position == target; });
		if (enemy != enemies.end())
		{
			attack(enemy);
			return true;
		}

		player = target;
		++turns;
		if (map[player.y][player.x] == 'R')
		{
			map[player.y][player.x] = '.';
			++relics;
			score += 100;
		}
		return true;
	}

	void attack(std::vector<Enemy>::iterator enemy)
	{
		enemy->health--;
		++turns;
		std::cout << '\a';
		if (enemy->health <= 0)
		{
			score += enemy->symbol == 'O' ? 75 : 40;
			enemies.erase(enemy);
		}
		else
		{
			health -= enemy->symbol == 'O' ? 2 : 1;
			if (health <= 0)
				gameOver = true;
		}
	}

	void moveEnemies()
	{
		for (Enemy& enemy : enemies)
		{
			if (std::abs(enemy.position.x - player.x) +
				std::abs(enemy.position.y - player.y) == 1)
			{
				health -= enemy.symbol == 'O' ? 2 : 1;
				if (health <= 0)
					gameOver = true;
				continue;
			}

			Position direction{};
			if (std::abs(player.x - enemy.position.x) >
				std::abs(player.y - enemy.position.y))
				direction = { player.x > enemy.position.x ? 1 : -1, 0 };
			else
				direction = { 0, player.y > enemy.position.y ? 1 : -1 };

			const Position next{ enemy.position.x + direction.x,
								 enemy.position.y + direction.y };
			if (isFloor(next) && !occupied(next))
				enemy.position = next;
		}
	}

	void usePotion()
	{
		if (potions > 0 && health < 12)
		{
			--potions;
			health = std::min(12, health + 5);
			++turns;
			moveEnemies();
		}
	}

	void checkVictory()
	{
		if (player == exitPosition && relics == relicGoal)
		{
			score += 500;
			won = true;
			gameOver = true;
		}
	}
};

int main()
{
	Game game;
	game.run();
	return 0;
}
