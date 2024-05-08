#include "MinesweeperWindow.h"
#include <iostream>
#include <random>

MinesweeperWindow::MinesweeperWindow(int x, int y, int width, int height, int mines, const string &title) : 
	// Initialiser medlemsvariabler, bruker konstruktoren til AnimationWindow-klassen
	AnimationWindow{x, y, width * cellSize, (height + 2) * cellSize, title},
	width{width}, height{height}, mines{mines}, minesLeftUser{mines},
	remainingTiles{height * width - mines}, 
	gameFeedback{{0, (height) * cellSize}, static_cast<unsigned int>(width * cellSize), static_cast<unsigned int>(cellSize), ""},
	gameRestart{{0, (height+1) * cellSize}, 4 * cellSize, cellSize, "Restart"},
	gameQuit{{(width - 4) * cellSize, (height+1) * cellSize}, 4 * cellSize, cellSize, "Quit"}
	{
	
	// Legg til alle tiles i vinduet
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			tiles.emplace_back(new Tile{ Point{j * cellSize, i * cellSize}, cellSize});
			tiles.back()->setCallback(std::bind(&MinesweeperWindow::cb_click, this));
			auto temp = tiles.back().get();
			add(*temp); 
		}
	}

	// Legger til miner paa tilfeldige posisjoner
	int i = 0;
	random_device rd;
	default_random_engine generator(rd());
	uniform_int_distribution<int> distribution(0, tiles.size());
	while (i < mines) {
		int pos = distribution(generator);
		if (!tiles[pos]->getIsMine()) {
			tiles[pos]->setIsMine(true);
			++i;
		}
	}

	//Legger til feltet for feedback under spillebrettet
	//Det starter med aa vise hvor mange miner som er igjen
	gameFeedback.setText("Mines left: " + to_string(minesLeftUser));
	add(gameFeedback);

	// Legger til buttons for å avslutte og restarte spillet
	gameRestart.setCallback(std::bind(&MinesweeperWindow::cb_restart, this));
	gameRestart.setLabelColor(Color::black);
	add(gameRestart);
	gameQuit.setCallback(std::bind(&MinesweeperWindow::cb_quit, this));
	gameQuit.setLabelColor(Color::black);
	add(gameQuit);
	gameRestart.setVisible(false);
	gameQuit.setVisible(false);

}

int MinesweeperWindow::countMines(vector<Point> points) const {
	int adjacentMines = 0;
	for (Point p : points) {
		if (at(p)->getIsMine()) {
			++adjacentMines;
		}
	}
	return adjacentMines;

	// Alternativt <algorithm>
	//  return std::count_if(points.begin(), points.end(), [this](auto point){
	//  	return at(point)->getIsMine();
	//  });
};
vector<Point> MinesweeperWindow::adjacentPoints(Point xy) const {
	vector<Point> points;
	for (int di = -1; di <= 1; ++di) {
		for (int dj = -1; dj <= 1; ++dj) {
			if (di == 0 && dj == 0) {
				continue;
			}

			Point neighbour{ xy.x + di * cellSize,xy.y + dj * cellSize };
			if (inRange(neighbour)) {
				points.push_back(neighbour);
			}
		}
	}

	return points;
}

void MinesweeperWindow::openTile(Point xy) {
	
	shared_ptr<Tile>& tile = at(xy);

	// Reagerer kun hvis Tile er lukket
	if (tile->getState() != Cell::closed) {
		return;
	}

	// Hvis Tilen er en mine er spillet over
	if (tile->getIsMine()) {
		gameLost();
		return;
	}

	tile->open();
	remainingTiles--;

	vector<Point> adjPoints = adjacentPoints(xy);
	int adjMines = countMines(adjPoints);

	if (adjMines > 0) { 
		// Hvis det er miner i naerheten setter vi labelen
		tile->setAdjMines(adjMines);
	}
	else {
		// Hvis det ikke er noen miner i naerheten vil vi aapne tilene rundt rekursivt 
		for (Point p : adjPoints) {
			openTile(p);
		}
	}

	if (remainingTiles == 0 && !gameIsWon) {
		gameWon();
		return;
	}
}

void MinesweeperWindow::flagTile(Point xy) {

	shared_ptr<Tile>& tile = at(xy);
	tile->flag();
	if (tile->getState() == Cell::flagged) {
		// Hvis den ble flagget markerer vi at brukeren har en mindre mine igjen aa flagge
		minesLeftUser--;
	}
	else if(tile->getState() == Cell::closed) {
		// Hvis ikke markerer vi at brukeren har en mer mine igjen aa flagge
		minesLeftUser++;
	}

	// Oppdaterer feedback teksten
	gameFeedback.setText(("Mines left: " + to_string(minesLeftUser)));
}

void MinesweeperWindow::click()
{
	Point xy{this->get_mouse_coordinates()};
	std::cout << xy.x << " " << xy.y <<": " << xy.x / (cellSize) + (xy.y / cellSize) * width<<"\n";

	if (!inRange(xy) || gameIsLost || gameIsWon) {
		return;
	}
	if (this->is_left_mouse_button_down()) {
		openTile(xy);
	}
	else if(this->is_right_mouse_button_down()){
		flagTile(xy);
	}
}

void MinesweeperWindow::restart() {

	for (auto tile : tiles) {
		tile->reset();
	}

	// Fjerner spillsluttmenyen
	gameRestart.setVisible(false);
	gameQuit.setVisible(false);

	// Plasser nye miner
	int mineCount = 0;
	while (mineCount < mines) {
		int pos = rand() % tiles.size();
		if (!tiles[pos]->getIsMine()) {
			tiles[pos]->setIsMine(true);
			++mineCount;
		}
	}

	// Nullstiller variabler
	remainingTiles = width * height - mines;
	gameIsLost = false;
	gameIsWon = false;
	minesLeftUser = mines;
	gameFeedback.setText(("Mines left: " + to_string(minesLeftUser)));
};

void MinesweeperWindow::gameLost() {
	gameIsLost = true;
	for (auto tile : tiles) {
		if (tile->getIsMine()) {
			tile->open();
		}
	}

	// Legger til meny for at spillet er tapt
	addGameEndMenu("Game Over");
};

void MinesweeperWindow::gameWon() { 
	gameIsWon = true;

	for (auto tile : tiles) {
		if (tile->getState() == Cell::closed) {
			tile->flag();
		}
	}
	// Legger til meny for at spillet er vunnet
	addGameEndMenu("Game won");
};

void MinesweeperWindow::addGameEndMenu(string s) {
	// Gir feedback om at spillets resultat
	gameFeedback.setText(s);

	//Legg til quit og restart knapp
	gameRestart.setVisible(true);
	gameQuit.setVisible(true);
}
