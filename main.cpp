#include <QCoreApplication>

#include <thread>
#include <utils/timer/timer.h>
#include <clients/vision/visionclient.h>
#include <clients/referee/refereeclient.h>
#include <clients/actuator/actuatorclient.h>
#include <clients/replacer/replacerclient.h>
#include <stdio.h>

#include "robot.h"
#include "utils.h"
#include "decision.h"
#include "vision.h"
#include "knn.h"
#include "QThread"
#include "iostream"
#include "log.h"
#include "config.h"
#include "positions.h"

#define PENALTY_KICK_GOALKEEPER_POS 0.75

using namespace std;

int main(int argc, char *argv[]) {
    std::string team(argv[1]);
    std::cout << team << std::endl << std::endl;
    //Core::Log::Init();
    Core::Config::Init();
    Position::Init(CONFIG_VAR("defenderDivisions"));
    //CORE_INFO("Code initialized");
    std::cout << "System initialized" << std::endl;
    QCoreApplication a(argc, argv);

    // Starting timer
    Timer timer;

    // Creating client pointers
    VisionClient *visionClient = new VisionClient("224.0.0.1", 10002);
    RefereeClient *refereeClient = new RefereeClient("224.5.23.2", 10003);
    ReplacerClient *replacerClient = new ReplacerClient("224.5.23.2", 10004);
    ActuatorClient *actuatorClient = new ActuatorClient("127.0.0.1", 20011);


    // Setting our color as BLUE at left side
    VSSRef::Color ourColor = VSSRef::Color::YELLOW;
    if(team == "blue") {
        ourColor = VSSRef::Color::BLUE;
    }
    bool ourSideIsLeft;

    if(ourColor == VSSRef::Color::BLUE){
        ourSideIsLeft = true;
    }
    if(ourColor == VSSRef::Color::YELLOW){
        ourSideIsLeft = false;
    }

    // Desired frequency (in hz)
    float freq = 60.0;

    // Setting actuator and replacer teamColor
    actuatorClient->setTeamColor(ourColor);
    replacerClient->setTeamColor(ourColor);

    double width,length;
    width = 1.3/2.0;
    length = 1.7/2.0;

    KNN Knn;

    Knn.load_database();

    vision *Vision;
    gamewindow GameWindow;
    GameWindow.setStrategy(atoi(argv[2]));
    Vision = new vision;
    vector<robot> rinobot; //___

    dataState bola;

    rinobot.resize(3);
    Point2f centroidDef;
    Point2f centroidAtk;

    centroidAtk.y = 1.3/2*100;
    centroidDef.y = 1.3/2*100;
    centroidAtk.x = 1.6*100;
    centroidDef.x = 0.1*100;

    Vision->setCentroidAtk(centroidAtk);
    Vision->setCentroidDef(centroidDef);

    int robots_n;
    while(1) {
        // Start timer
        timer.start();

        // Running vision and referee clients
        visionClient->run();
        refereeClient->run();

        fira_message::Frame detection = visionClient->getLastEnvironment().frame();
        fira_message::Ball ball = detection.ball();
        if(ourColor == VSSRef::Color::BLUE){
            robots_n =  detection.robots_blue_size();
        }
        if(ourColor == VSSRef::Color::YELLOW){
            robots_n =  detection.robots_yellow_size();
        }
        if(ourColor == VSSRef::Color::BLUE){
            bola.pos.x = (length+ball.x())*100;
            bola.pos.y = (width+ball.y())*100;
        }
        if(ourColor == VSSRef::Color::YELLOW){
            bola.pos.x = fabs((-length+ball.x())*100);
            bola.pos.y = fabs((-width+ball.y())*100);
        }

        if(ourColor == VSSRef::Color::BLUE){
            bola.vel = Point2f(ball.vx()*100,ball.vy()*100);
        }
        if(ourColor == VSSRef::Color::YELLOW){
            bola.vel = Point2f(ball.vx()*100*(-1),ball.vy()*100*(-1));
        }


        if(ourColor == VSSRef::Color::BLUE){
            for (int i = 0; i < robots_n; i++) {
                fira_message::Robot robot = detection.robots_blue(i);
                Point2f pos;
                Point2f vel;
                pos.x = (length+robot.x())*100;
                pos.y = (width+robot.y())*100;//convertendo para centimetros
                double ang = ajustaAngulo(robot.orientation()*180/PI);
                vel = Point2f(robot.vx()*100,robot.vy()*100);
                rinobot[i].setPosition(pos,ang, vel); //___
            }
        }

        if(ourColor == VSSRef::Color::YELLOW){
            for (int i = 0; i < robots_n; i++) {
                fira_message::Robot robot = detection.robots_yellow(i);
                Point2f pos;
                pos.x = (fabs(-length+robot.x()))*100;
                pos.y = (fabs(-width+robot.y()))*100;//convertendo para centimetros
                double ang = ajustaAngulo(robot.orientation()*180/PI+180);
                Point2f vel = Point2f(robot.vx()*100*(-1),robot.vy()*100*(-1)); //___
                rinobot[i].setPosition(pos,ang, vel); //___
            }
        }

        Vision->setBall(bola);
        Vision->setRobots(rinobot); //___
        if(robots_n != 0 && refereeClient->getLastFoul() == VSSRef::Foul::GAME_ON)
            GameWindow.updateInfo(Vision->getRobots(),Vision->getEnemy(),Vision->getCentroidDef(), Vision->getCentroidAtk(), Vision->getBall(), Knn);

        // Sending robot commands for robot 0, 1 and 2
        Point2f Velocidades[3];
        GameWindow.EnviaVelocidades(Velocidades);

        //        for(int i = 0; i < 3; i++){
        //            actuatorClient->sendCommand(i, Velocidades[i].x, Velocidades[i].y);
        //        }

        if(GameWindow.get_strategy() == 0 || GameWindow.get_strategy() == 4) // FIXED && Libero
        {
            if(refereeClient->getLastFoul() == VSSRef::Foul::KICKOFF) {
                if(refereeClient->getLastFoulColor() == VSSRef::Color::BLUE){
                    replacerClient->placeRobot(2, ourSideIsLeft ? -0.08 : 0.24, 0, ourSideIsLeft ? 20 : 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.4 : 0.4, 0, 90);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -PENALTY_KICK_GOALKEEPER_POS : PENALTY_KICK_GOALKEEPER_POS, 0, 90);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulColor() == VSSRef::Color::YELLOW){

                    replacerClient->placeRobot(2, ourSideIsLeft ? -0.24 : 0.08, 0, ourSideIsLeft ? 0 : 20);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.4 : 0.4, 0, 90);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -PENALTY_KICK_GOALKEEPER_POS : PENALTY_KICK_GOALKEEPER_POS, 0, 90);
                    replacerClient->sendFrame();
                }
            }
            if(refereeClient->getLastFoul() == VSSRef::Foul::PENALTY_KICK){
                if(refereeClient->getLastFoulColor() == VSSRef::Color::BLUE){
                    replacerClient->placeRobot(2, ourSideIsLeft ? 0.3 : -0.1, 0, -25);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.4 : -0.1, -0.2, 90);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -PENALTY_KICK_GOALKEEPER_POS : PENALTY_KICK_GOALKEEPER_POS, 0, 90);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulColor() == VSSRef::Color::YELLOW){

                    replacerClient->placeRobot(2, ourSideIsLeft ? 0.1 : -0.3, 0, 25);
                    replacerClient->placeRobot(1, ourSideIsLeft ? 0.1 : 0.4 , -0.1, 90);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -PENALTY_KICK_GOALKEEPER_POS : PENALTY_KICK_GOALKEEPER_POS, 0.05, 90);
                    replacerClient->sendFrame();
                }
            }

            if(refereeClient->getLastFoul() == VSSRef::Foul::FREE_BALL){
                if(refereeClient->getLastFoulQuadrant() == VSSRef::Quadrant::QUADRANT_1){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  0.16 : 0.3, ourSideIsLeft ? 0.4 : -0.04 , 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ?  -0.38  : 0.6, 0.4 , 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -0.71 : 0.71, 0, 90);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulQuadrant() == VSSRef::Quadrant::QUADRANT_2){
                    replacerClient->placeRobot(1, ourSideIsLeft ?  -0.6 : 0.38, 0.4, 0);
                    replacerClient->placeRobot(2, ourSideIsLeft ?  -0.3 : -0.16, ourSideIsLeft ? -0.15: 0.4, 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -0.71 : 0.71, 0, 90);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulQuadrant() == VSSRef::Quadrant::QUADRANT_3){
                    replacerClient->placeRobot(1, ourSideIsLeft ?  -0.6 : 0.38, -0.4, 0);
                    replacerClient->placeRobot(2, ourSideIsLeft ?  -0.3 : -0.16, ourSideIsLeft ? 0.15: -0.4, 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -0.71 : 0.71, 0, 90);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulQuadrant() == VSSRef::Quadrant::QUADRANT_4){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  0.16 : 0.3, ourSideIsLeft ? -0.4 : +0.04 , 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ?  -0.38  : 0.6, -0.4 , 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -0.71 : 0.71, 0, 90);
                    replacerClient->sendFrame();
                }
            }

            if(refereeClient->getLastFoul() == VSSRef::Foul::GOAL_KICK){
                if(refereeClient->getLastFoulColor() == VSSRef::Color::BLUE){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  -0.45 : -0.2, -ball.y() > 0 ? -0.55 : 0.55 , 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ?  -0.40 : 0.38 , 0 , 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -0.66 : 0.71, ourSideIsLeft ? (ball.y() > 0 ? 0.35 : -0.35): 0 , 0);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulColor() == VSSRef::Color::YELLOW){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  0.2 : 0.45, 0.2,0);
                    replacerClient->placeRobot(1, ourSideIsLeft ?  -0.38 : 0.68 , ourSideIsLeft ? 0: (ball.y() > 0 ? 0.35 : -0.35) , 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ?  -0.71 : 0.66 , 0.35, 0);
                    replacerClient->sendFrame();
                }
            }

        }

        else if(GameWindow.get_strategy() == 1) // FULL ATK
        {
            if(refereeClient->getLastFoul() == VSSRef::Foul::PENALTY_KICK){
                if(refereeClient->getLastFoulColor() == VSSRef::Color::BLUE){
                    replacerClient->placeRobot(2, ourSideIsLeft ? 0.3 : -0.1, ourSideIsLeft ? 0 : 0.4, -20);
                    replacerClient->placeRobot(0, ourSideIsLeft ? -0.38 : -0.1, 0, 90);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.1 : 0.72, ourSideIsLeft ? -0.2: 0, ourSideIsLeft ? 0 : 90);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulColor() == VSSRef::Color::YELLOW){
                    replacerClient->placeRobot(2, ourSideIsLeft ? 0.1 : -0.3, ourSideIsLeft ? 0.2 : 0, -20);
                    replacerClient->placeRobot(0, ourSideIsLeft ? 0.1 : 0.38, 0, 90);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.72 : 0.1, ourSideIsLeft ? 0.05: -0.2, ourSideIsLeft ? 90 : 0);
                    replacerClient->sendFrame();
                }
            }

            if(refereeClient->getLastFoul() == VSSRef::Foul::FREE_BALL){
                if(refereeClient->getLastFoulQuadrant() == VSSRef::Quadrant::QUADRANT_1){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  0.14 : 0.3, ourSideIsLeft ? 0.4 : -0.04 , 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ?  -0.38  : 0.62, 0.4 , 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.71 : 0.71, 0, 90);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulQuadrant() == VSSRef::Quadrant::QUADRANT_2){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  -0.6 : -0.16, 0.4, 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ?  -0.3 : 0.38, ourSideIsLeft ? -0.15: 0.4, 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.71 : 0.71, 0, 90);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulQuadrant() == VSSRef::Quadrant::QUADRANT_3){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  -0.6 : -.016, -0.4, 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ?  -0.3 : 0.38, ourSideIsLeft ? 0.15: -0.4, 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.71 : 0.71, 0, 90);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulQuadrant() == VSSRef::Quadrant::QUADRANT_4){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  0.14 : 0.3, ourSideIsLeft ? -0.4 : +0.04 , 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ?  -0.38  : 0.62, -0.4 , 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.71 : 0.71, 0, 90);
                    replacerClient->sendFrame();
                }
            }

            if(refereeClient->getLastFoul() == VSSRef::Foul::GOAL_KICK){
                if(refereeClient->getLastFoulColor() == VSSRef::Color::BLUE){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  -0.45 : -0.25, ourSideIsLeft ? 0.6 : 0.35 , 0);
                    replacerClient->placeRobot(0, ourSideIsLeft ?  -0.68 : 0.38 , ourSideIsLeft ? (ball.y() > 0 ? 0.35 : -0.35): 0 , 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ? -0.55 : -0.25, ourSideIsLeft ? 0 : -0.35, 0);
                    replacerClient->sendFrame();
                }
                if(refereeClient->getLastFoulColor() == VSSRef::Color::YELLOW){
                    replacerClient->placeRobot(2, ourSideIsLeft ?  0.25 : 0.45, ourSideIsLeft ? 0.35 : 0.6,0);
                    replacerClient->placeRobot(0, ourSideIsLeft ?  -0.38 : 0.68 , ourSideIsLeft ? 0: (ball.y() > 0 ? 0.35 : -0.35) , 0);
                    replacerClient->placeRobot(1, ourSideIsLeft ?  0.25 : 0.55 ,ourSideIsLeft ? -0.35 : 0, 0);
                    replacerClient->sendFrame();
                }
            }

        }

        if(refereeClient->getLastFoul() == VSSRef::Foul::GAME_ON){
            for(int i = 0; i < 3; i++){
                actuatorClient->sendCommand(i, Velocidades[i].x, Velocidades[i].y);
            }
        }
        else{
            if(refereeClient->getLastFoul() == VSSRef::Foul::STOP){
                for(int i = 0; i < 3; i++){
                    actuatorClient->sendCommand(i, 0, 0);
                }
            }
        }
        if(refereeClient->getLastFoul() == VSSRef::Foul::HALT){
            for(int i = 0; i < 3; i++){
                actuatorClient->sendCommand(i, 0, 0);
            }
        }
        if(refereeClient->getLastFoul() == VSSRef::Foul::KICKOFF){
            replacerClient->placeRobot(2, ourSideIsLeft ? -0.24 : 0.24, 0, 0);
            replacerClient->placeRobot(1, ourSideIsLeft ? -0.38 : 0.38, 0, 90);
            replacerClient->placeRobot(0, ourSideIsLeft ? -0.70 : 0.70, 0, 90);
            replacerClient->sendFrame();
        }

        
        // Stop timer
        timer.stop();

        // Sleep for remainingTime
        long remainingTime = (1000 / freq) - timer.getMiliSeconds();
        //std::this_thread::sleep_for(std::chrono::milliseconds(remainingTime));
    }

    // Closing clients
    visionClient->close();
    refereeClient->close();
    replacerClient->close();
    actuatorClient->close();

    return a.exec();
}
