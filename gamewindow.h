#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H
#include "utils.h"
#include "decision.h"
#include "gamefunctions.h"
#include "knn.h"
#include "mover.h"
#include <string>
#include <limits.h>
#include <unistd.h>
#include "config.h"



class gamewindow
{
public:
    gamewindow();

    vector<Point2f> posBall, posRobot0, posRobot1, posRobot2;
    Decision deecision;

    void updateInfo(vector<robot>, vector<robot>, Point2f, Point2f, dataState, KNN k);
    void updateRunFunctions(vector<robot>, Point2f, Point2f, dataState);
    GameFunctions *robotFunctions[3];
    Mover *mover[3];
    int get_strategy();

    void setStrategy(int strategy) {
        deecision.setStrategy(strategy);
    }

    void EnviaVelocidades(Point2f Velocidades[3]);
    json config;
private:
};

#endif // GAMEWINDOW_H
