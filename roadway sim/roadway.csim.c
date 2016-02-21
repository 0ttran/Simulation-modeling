//Tien Tran, 861045781

#include <iostream>
#include <vector>
#include "cpp.h"

#include <string.h>
using namespace std;


#define MEAN 1.0
#define TINY 1.e-20		// a very small time period
#define cells 120
#define numCars 1
#define ENDTIME 10000

double D[cells], S[cells];
double x;

facility_set* road = new facility_set("road", cells);

//Car class that contains sits index, speed, movingstatus
class car{

private:
    int speed;
    int index;
    double nextIndex;
    int moving;
public:
    car();
    int getSpeed();
    int getIndex();
    void setSpeed(int spd);
    void setIndex(int indx);
    void setMoving(int flg);
};
 
car::car(){
    speed = rand() % 5 + 2;
    index = 1;
    moving = 0;
}

int car::getSpeed(){
    return speed;
}

int car::getIndex(){
    return index;
}

void car::setSpeed(int spd){
    speed = spd;
}

void car::setIndex(int indx){
    index = indx;
}

void car::setMoving(int flg){
    moving = flg;
}

double calculateD(int carIndex, int getNext, int speed);
double getSpeed(double old_clock);
double getWaitTime(int speed);
void carProc(car* C, int flg);

//Borrowed this function to handle negative numbers
long long int mod(long long int a, long long int b) {
    long long int ret = a % b;
    if (ret < 0)
        ret += b;
    return ret;
}

extern "C" void sim(){
    create("sim");

    //trace_on();
    vector<car*> carList;
    //trace_on();
    for(unsigned i = 0; i < cells; i++) //Initialize D to be all 0
        D[i] = 0;
    
    car* car1 = new car();
    carList.push_back(car1);

    if(numCars != cells){
        int i;
        //for(i = 0; i < numCars; i++){ 
        carProc(car1, 1);
            //hold(2); //time between each car?
        //}
    }
    else
        cout << "Deadlock!" << endl;
    
    hold(1440);

    //Deallocate memory
    for(vector<car*>::iterator it = carList.begin(); it != carList.end(); it++)
        delete *it;
    carList.clear();
}

void carProc(car* C, int flg){
    create("carProc");
    int speed = rand() % 5 + 2;
    double old_time = clock;
    int moving = 1;
    int next = C->getIndex() + 1;
    //Initial spot reserve
    (*road)[C->getIndex()].reserve(); //Front
    (*road)[C->getIndex() - 1].reserve(); //Back

    while(clock < ENDTIME){
        //cout << "clock: " << clock << endl;
        //Change speed of car every 1-2mins
        if((clock - old_time) > uniform(60, 120))
            speed = rand() % 5 + 2;
        if((*road)[next % cells].status() == 0){
            //cout << "in: " << C->getIndex() << endl;
            C->setIndex((C->getIndex() + 1) % cells);
            D[C->getIndex()] = calculateD(C->getIndex(),(C->getIndex() + 1) % cells, speed);
            cout << "waitfor: " << D[mod(C->getIndex()-2,cells)] << endl;
            (*road)[C->getIndex()].reserve(); 
            hold(D[mod(C->getIndex()-2,cells)]);
            //cout << "release : " << mod(carIndex -2, cells) << endl;
            (*road)[mod(C->getIndex()-2,cells)].release();
            C->setMoving(1);
            next = C->getIndex() + 1;
        }
        else{
            C->setMoving(0); 
            speed = 0;
            old_time = clock;
        }
        
    }
}

//Function calculates the depature time 
double calculateD(int carIndex, int getNext, int speed){ 
    
    //Checks the next cell if busy and determine departure time
    if((*road)[getNext].status() > 0){
        if((D[getNext] + TINY) > (clock + getWaitTime(speed))){
            double tmp = D[getNext] + TINY;
            return tmp;
        }
        else{
            return clock + getWaitTime(speed);
        }
        
    }
    else{
        cout << "here" << endl;
        return clock + getWaitTime(speed);
    }
}

double getWaitTime(int speed){
    if(speed == 1)
        return 1.5;
    else if(speed == 2)
        return 11/6;
    else if(speed == 3)
        return 1/2;
    else if(speed == 4)
        return 1/3;
    else if(speed == 5)
        return 1/4;
}
