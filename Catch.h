#ifndef CATCH_H
#define CATCH_H

#include <QMainWindow>

#include "Cell.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Catch;
}
QT_END_NAMESPACE

class Player;

class Catch : public QMainWindow {
    Q_OBJECT

public:
    Catch(QWidget *parent = nullptr);
    virtual ~Catch();
    Cell* cellAt(int row, int col) const;//

signals:
    void turnEnded();

private:
    Ui::Catch *ui;
    Player* m_player;
    Cell* m_board[8][8];
    QList<Cell::State> emptycells(Cell* cell);

private slots:
    void play(int id);
    void switchPlayer();
    void reset();
    void showAbout();
    bool isWithinBounds(Cell* cell);

    bool endGame(int id, Player* player);
    int playerWins();

    void updateSelectables(bool over);
    void updateStatusBar();

};

#endif // CATCH_H
