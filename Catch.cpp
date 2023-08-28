#include "Catch.h"
#include "ui_Catch.h"
#include "Player.h"

#include <QDebug>
#include <QMessageBox>
#include <QActionGroup>
#include <QSignalMapper>
#include <QList>
#include <QQueue>
#include <iostream>

Catch::Catch(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::Catch),
    m_player(Player::player(Player::Red)) {

    ui->setupUi(this);

    QObject::connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(reset()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAbout()));

    QSignalMapper* map = new QSignalMapper(this);
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            QString cellName = QString("cell%1%2").arg(row).arg(col);
            Cell* cell = this->findChild<Cell*>(cellName);
            Q_ASSERT(cell != nullptr);
            Q_ASSERT(cell->row() == row && cell->col() == col);

            m_board[row][col] = cell;

            int id = row * 8 + col;
            map->setMapping(cell, id);
            QObject::connect(cell, SIGNAL(clicked(bool)), map, SLOT(map()));
            QObject::connect(cell, SIGNAL(mouseOver(bool)), this, SLOT(updateSelectables(bool)));
        }
    }
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QObject::connect(map, SIGNAL(mapped(int)), this, SLOT(play(int)));
#else
    QObject::connect(map, SIGNAL(mappedInt(int)), this, SLOT(play(int)));
#endif

    // When the turn ends, switch the player.
    QObject::connect(this, SIGNAL(turnEnded()), this, SLOT(switchPlayer()));

    this->reset();

    this->adjustSize();
    this->setFixedSize(this->size());
}

Catch::~Catch() {
    delete ui;
}
Cell* Catch::cellAt(int row, int col) const {
    return row >= 0 && row < 8 &&
                   col >= 0 && col < 8 ? m_board[row][col] : nullptr;

}

void Catch::play(int id) {
    Cell* cell = m_board[id / 8][id % 8];
    Cell* Vneighbor = m_board[(id / 8)+1][id % 8];
    Cell* Hneighbor = m_board[id / 8][(id % 8)+1];


    Player* red = Player::player(Player::Red);
    Player* blue = Player::player(Player::Blue);

    if (cell == nullptr || !cell->isSelectable())
        return;

    cell->setState(Cell::Blocked);
    if(m_player == blue){
        Vneighbor->setState(Cell::Blocked);

    }
    if(m_player == red){
        Hneighbor->setState(Cell::Blocked);

    }
    //Capture
    QList<QList<bool>> visited(8, QList<bool>(8, false));

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {

            QList<Cell*> grupo;
            QList<Cell*> limite;

            if (!m_board[row][col]->isEmpty() || visited[row][col]){
                continue;
            }

            limite.push_back(m_board[row][col]);

            while (!limite.empty()) {

                Cell* celulaAtual = limite.back();

                limite.pop_back();



                if (isWithinBounds(celulaAtual)){
                    continue;
                }
                if (visited[celulaAtual->row()][celulaAtual->col()] || !m_board[celulaAtual->row()][celulaAtual->col()]->isEmpty()){
                    continue;
                }
                visited[celulaAtual->row()][celulaAtual->col()] = true;


                grupo.push_back(celulaAtual);

                if (celulaAtual->row() > 0){
                    // Vizinho Norte
                    limite.push_back(m_board[celulaAtual->row() - 1][celulaAtual->col()]);
                }
                if (celulaAtual->row() < 7){
                    // Vizinho Sul
                    limite.push_back(m_board[celulaAtual->row() + 1][celulaAtual->col()]);
                }
                if (celulaAtual->col() > 0){
                    // Vizinho Oeste
                    limite.push_back(m_board[celulaAtual->row()][celulaAtual->col() - 1]);
                }
                    // Vizinho Leste
                if (celulaAtual->col() < 7){
                    limite.push_back(m_board[celulaAtual->row()][celulaAtual->col() + 1]);
                }
            }

            if (grupo.size() >= 1 && grupo.size() <= 3) {
                for (Cell* cell : grupo) {
                    cell->setPlayer(m_player);
                    m_player->incrementCount();
                }

            }
        }
    }



    updateStatusBar();

    if(endGame(id, m_player)){
        // Exibir mensagem de fim de jogo
        QString message;
        int result = playerWins();
        if (result == 1) {
            message = (tr("Parabéns, o Jogador Vermelho venceu por (%1 a %2)").arg(m_player->count()).arg(m_player->other()->count()));
        } else if (result == 2) {
            message = (tr("Parabéns, o Jogador Azul venceu por (%1 a %2)").arg(m_player->count()).arg(m_player->other()->count()));
        } else if (result == 3) {
            message = (tr("O jogo empatou em (%1 a %2)").arg(m_player->count()).arg(m_player->other()->count()));
        }

        QMessageBox::information(this, "Fim de jogo", message);

        this->reset();

    }

    emit turnEnded();





}

int Catch::playerWins() {
    int cellsP1 = 0;
    int cellsP2 = 0;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Cell* cell = m_board[row][col];
            if (cell->isCaptured()) {
                if (cell->player()->type() == Player::Red) {
                    cellsP1++;
                }else if (cell->player()->type() == Player::Blue) {
                    cellsP2++;
                }
            }
        }
    }

    if (cellsP1 > cellsP2) {
        return 1;
    }else if(cellsP2 > cellsP1){
        return 2;
    }else{
        return 3;
    }
}

bool Catch::endGame(int id, Player* player) {
    if(m_player->type() == Player::Red){
        for(int row = 0; row < 8; ++row){
            for(int col = 0; col < 7; ++col){
                if(m_board[row][col]->isEmpty() && m_board[row][col + 1]->isEmpty())
                    return false;
            }
        }
    }

    if(m_player->type() == Player::Blue){
        for(int row = 0; row < 7; ++row){
            for(int col = 0; col < 8; ++col){
                if(m_board[row][col]->isEmpty() && m_board[row+1][col]->isEmpty())
                    return false;
            }
        }
    }

    return true;
}

bool Catch::isWithinBounds(Cell* cell){
    return cell->row() < 0 || cell->row() >= 8 || cell->col() < 0 || cell->col() >= 8;

}


void Catch::switchPlayer() {

    // Switch the player.
    m_player = m_player->other();

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Catch::reset() {
    // Reset board.
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Cell* cell = m_board[row][col];
            cell->reset();
        }
    }

    // Reset the players.
    Player* red = Player::player(Player::Red);
    red->reset();

    Player* blue = Player::player(Player::Blue);
    blue->reset();

    m_player = red;

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Catch::showAbout() {
    QMessageBox::information(this, tr("Sobre"), tr("Catch\n\nAndrei Rimsa Alvares - andrei@cefetmg.br"));
}

void Catch::updateSelectables(bool over) {
    Cell* cell = qobject_cast<Cell*>(QObject::sender());
    Player* red = Player::player(Player::Red);
    Player* blue = Player::player(Player::Blue);
    Cell* Vneighbor = this->cellAt(cell->row()+1,cell->col());
    Cell* Hneighbor = this->cellAt(cell->row(),cell->col()+1);

    Q_ASSERT(cell != nullptr);

    if (over) {
        if (cell->isEmpty()){
            if(m_player == red && cell->col() != 7 && Hneighbor->isEmpty()){
                cell->setState(Cell::Selectable);
                Hneighbor->setState(Cell::Selectable);
            }
            else if(m_player == blue && cell->row() != 7 && Vneighbor->isEmpty()){
                cell->setState(Cell::Selectable);
                Vneighbor->setState(Cell::Selectable);
            }
        }

    } else {
        if (cell->isSelectable()){
            if(m_player == red && cell->col() != 7  && Hneighbor->isSelectable()){
                cell->setState(Cell::Empty);
                Hneighbor->setState(Cell::Empty);
            }
            else if(m_player == blue && cell->row() != 7 && Vneighbor->isSelectable()){
                cell->setState(Cell::Empty);
                Vneighbor->setState(Cell::Empty);
            }

        }
    }
}

void Catch::updateStatusBar() {
    ui->statusbar->showMessage(tr("Vez do %1 (%2 a %3)")
                                   .arg(m_player->name()).arg(m_player->count()).arg(m_player->other()->count()));
}
