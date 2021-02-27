#ifndef MOVER_H
#define MOVER_H

#include <QMutex>
#include <QThread>
#include "gamefunctions.h"

class Mover: public QThread
{
    Q_OBJECT
public:
    Mover();
    void setVMax(float);
    float getVMax();
    float getLVel();
    float getRVel();
    void goalkeeper();
    void defender();
    void striker();
    void fake9();
    void midfield();
    void wing();
    void volante();
    void fuzzy();
    void impostor();
    void libero();
    void newstriker();
    void offdefender();
    void setGameFunctions(GameFunctions *, GameFunctions *, GameFunctions *);
    void setRobots(vector<robot>);
    void setAreas(Point2f, Point2f);
    void setAgainstTheTeam(bool);
    void setDirection(bool);
    void setBall(dataState);
    void setGains(MatrixXd);
    MatrixXd getGains();
    void rotate();
    void rotateInv();
    void kickRotate();
    void kickPenalty();
    void updateGains();
    void atkSituation();
    void atkSituationInv();
    float twiddle();
    clock_t clockStart;
    float vMax = 70;
    float kp;
    float kd;
    float alphaS;
    Point2f posTemp;
    Point2f prevGoal;
    float temp = 0;
    bool inverte=false;
    clock_t clockInvert, clockTroca, clockAceleration;
    float tempoTroca = 0;
    bool sentido = false;
    void setStrategy(int);
    int getStrategy();
    void kickGoalKeeper();
    void setIndex(int);
    ~Mover();
protected:
    void run();
private:
    GameFunctions *robotFunctions[NUMROBOTS];
    QMutex mutex;
    vector<robot> teamRobot;
    int indexRobot;
    Point2f centroidDef, centroidAtk;
    dataState ball;
    MatrixXd robotGains;

    float l = 0.0285;
    float alpha;
    float lastAlpha;
    float lastVel;

    bool robotDirection = true;
    int robotObstCont = 0;
    int robotTimeCont = 0;
    Point2f lastRobot;

    bool againstTheTeam = false;
    bool firstAceleration = true;
    bool airball;

    float offLine = 115;
    float distGiro = 6;
    float distGiroGoleiro = 8;
    float velGiroLado = 0.8;
    float velGiroAtk = 0.5;
    float velGiroGol = 1;
    float velGiroPenalty = 2;
    float lVel;
    float rVel;

    int strategy;
};

#endif // MOVER_H
