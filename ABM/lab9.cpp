
//Name: Tien Tran
//SID: 86104581

// CSC 270 simulation example
// Adapted March 1996 by J. Clarke from Turing original by M. Molle

// This version edited slightly for the fall 1997 course notes.

// What system are we on?
//
// The system matters only because random-number generating functions
// tend to be system-dependent. To set the system, comment out the
// irrelevant choices and make sure the one you're using is not
// commented out. (A sensible alternative would be to comment out
// all the system #defines and use a compiler flag instead.)
//
// If your system is not mentioned here, look up its random number
// generator and add it to the list. The GNU random number generators
// are certainly preferable to the TURBO (Turbo C++) ones.
#define GNU
//#define TURBO

#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <queue>
using namespace std;
#define BOOLEAN int
#define TRUE 1
#define FALSE 0

//-----------------------------------------------------------------------
// The time, a "keep going" control, and some constants of the model.
//
// There are several other global variables. Should we use them?
// Could we get rid of them? Where would they appear if we broke the
// program up into several separately-compiled modules? We certainly
// ought to do that!
//-----------------------------------------------------------------------

double simulationTime; // A major global variable: the current time.
BOOLEAN simulating; // Are we still going?

const double profit = .025; // dollars per litre
const double cost = 20;
vector<double> dataAvgLitre;
// the incremental cost in dollars to operate one pump for a day

//-----------------------------------------------------------------------
// Generating random numbers.
//-----------------------------------------------------------------------

// Prototypes for system-provided random number functions.
#if defined(GNU)
double erand48(unsigned short xsubi[3]);
#elif defined(TURBO)
int rand ();
#endif

class randStream {
// Manage one stream of random numbers.
//
// "nextrand" gives the floating-point number obtained from a
// new value in this stream. "xsubi" stores that latest value.
private:
	unsigned short xsubi[3];

public:
	randStream (unsigned short seed = 0);
	double nextrand ();
};

randStream * arrivalStream; // auto arrival times
randStream * litreStream; // number of litres needed
randStream * balkingStream; // balking probability
randStream * serviceStream; // service times

double randStream::nextrand ()
// Returns the next random number in this stream.
{
#if defined(GNU)
	return erand48 (xsubi);
#elif defined(TURBO)
	return ((double) rand())/((double) RAND_MAX);
#endif
}

randStream::randStream (unsigned short seed)
// Initialize the stream of random numbers.
{
#if defined(GNU)
	xsubi[0] = 0;
	xsubi[1] = 0;
	xsubi[2] = seed;
#elif defined(TURBO)
	// Turbo C doesn't provide a random-number generator with
	// explicit seeds. We will get along with just one stream.
	return;
#endif
}


//-----------------------------------------------------------------------
// Events
//
// Remember that events are not entities in the same sense as cars and
// pumps are, and the event queue does not have the same reality as the
// car queue. The event queue is a data structure without a real-world
// equivalent, while the car queue is real and you can see it. Events are
// not quite so imaginary, but they are certainly less "real" than cars.
//-----------------------------------------------------------------------

//----------------------------------------------------------------------
// Events--the general class on which the specific event types are based
//----------------------------------------------------------------------

class eventClass {
private:
	double localTime;

public:
	eventClass (double T = 0.0) {localTime = T;};
	inline double whatTime () {return localTime;}; // when it happens
	inline void setTime (const double newTime = 0) {localTime = newTime;};
	virtual void makeItHappen () {}; // the event routine
};

//Comparator class for eventClass
class compareTime{
public:
	bool operator()(eventClass* lhs, eventClass* rhs) const {
		//cout << "lhs: " << lhs->whatTime() << "  rhs: " << rhs->whatTime() << endl;
		return lhs->whatTime() > rhs->whatTime();
	}
	
};

//--------------------------------------------------
// The event list and utilities for manipulating it.
//--------------------------------------------------

class eventListClass {
private:
	priority_queue<eventClass*, vector<eventClass*>, compareTime> pq;

public:
	eventListClass () {};
	void insert (eventClass * event);
	eventClass * getNext ();
};

eventListClass * eventList;

void eventListClass::insert (eventClass * event)
{
	pq.push(event);
}

eventClass * eventListClass::getNext ()
{
	if(!(pq.empty())){
		eventClass* tmp = pq.top();
		pq.pop();
		return tmp;
	}
	else
		return NULL;
}

//---------------------------------------------------------------------------
// Entities: cars and pumps
//---------------------------------------------------------------------------

class carClass {
private:
	double localArrivalTime;
	double localLitresNeeded;

public:
	carClass();
	void setTime (double T) {localArrivalTime = T;};
	double arrivalTime () {return localArrivalTime;};
	double litresNeeded () {return localLitresNeeded;};
};

carClass::carClass ()
// The number of litres required is a property of a car, so it
// belongs in this class. It is also something the car "knows"
// when it arrives, so it should be calculated in the constructor.
//
// The distribution of litres required is uniform between 10 and 60.
{
	localLitresNeeded = 10 + litreStream -> nextrand() * 50;
};

class pumpClass {
private:
	carClass * localCarInService;
	double serviceTime ();

public:
	carClass * carInService () {return localCarInService;};
	void startService (carClass *);
};

double pumpClass::serviceTime()
// Determine how long the service will take. This is a property of the
// pump-car combination.
//
// Service times will have a near Normal distribution, where the mean is 150
// seconds plus 1/2 second per litre, and the standard deviation is 30 seconds.
// (It can be shown that the sum of 12 random numbers from the
// uniform distribution on (0,1) has a near Normal distribution
// with mean 6 and standard deviation 1.)
{
	if (carInService() == NULL) {
		cout << "Error! no car in service when expected\n";
		return -1.0;
	}

	double howLong = -6;

	for (int i = 1; i <= 12; i++)
		howLong += serviceStream -> nextrand();

	return 150 + 0.5 * localCarInService -> litresNeeded() + 30 * howLong;
}

//----------------------------------------
// The actual pumps at the service station
//----------------------------------------

class pumpStandClass {
private:
	typedef pumpClass * pumpListItem;
	pumpListItem * pumps; // A stack of pumps
	int numPumps;
	int topPump;

public:
	pumpStandClass (int);
	int howManyPumps () {return numPumps;}; // needed for statistics
	BOOLEAN existsAvailablePump () {return topPump >= 0;};
	pumpClass * getAvailablePump ();
	void releasePump (pumpClass *&);
};

pumpStandClass * pumpStand;

pumpStandClass::pumpStandClass (int N)
// Create set of available pumps and put all N pumps into it.
{
	if (N < 1) {
		cout << "Error! pump stand needs more than 0 pumps\n";
		return;
	}

	numPumps = N;
	pumps = new pumpListItem [N];
	topPump = N - 1;

	for (int j = 0; j < N; j++)
		pumps[j] = new pumpClass;
}

pumpClass * pumpStandClass::getAvailablePump ()
// Take a pump from the set of free pumps, and return a pointer to it.
{
	if (topPump < 0) {
		cout << "Error! no pump available when needed\n";
		return NULL;
	}

	return pumps[topPump--];
}

void pumpStandClass::releasePump (pumpClass *& P)
// Put pump P back on the available list.
// P is set to NULL, just to cause trouble if the caller tries
// to reuse this pump, which is now officially free.
{
	if (topPump >= numPumps) {
		cout << "Error! attempt to release a free pump\n";
		return;
	}

	pumps[++topPump] = P;
	P = NULL;
}

//---------------------------------------------------------
// Queue for waiting cars and utilities for manipulating it
//---------------------------------------------------------

class carQueueClass {
private:
	queue<carClass*> q;

	double totalEmptyQueueTime;
	int numCars;

public:
	carQueueClass ();
	int queueSize ();
	double emptyTime ();
	void insert (carClass * newestCar);
	carClass * getNext ();
	int getMaxCars();
};

carQueueClass * carQueue;

carQueueClass::carQueueClass ()
{

	numCars = 0;
	totalEmptyQueueTime = 0.0;
}

int carQueueClass::queueSize ()
{
	return q.size();
}

double carQueueClass::emptyTime ()
{
	if (q.size() > 0)
		return totalEmptyQueueTime;
	else
		return totalEmptyQueueTime + simulationTime;
}

void carQueueClass::insert (carClass * newestCar)
{

	if(q.empty()){
		q.push(newestCar);
		totalEmptyQueueTime += simulationTime;
	}
	else
		q.push(newestCar);

	//Update max cars 
	if(q.size() >= numCars)
		numCars = q.size();
	
}

carClass * carQueueClass::getNext ()
{

	if(!q.empty()){
		carClass* tmp = q.front();
		q.pop();

		if(q.size() == 0){
			totalEmptyQueueTime -= simulationTime;
		}

		return tmp;
	}
	else
		return NULL;
}

int carQueueClass::getMaxCars(){
	return numCars;
}

//-------------------------------------
// Collecting and displaying statistics
//-------------------------------------

class statsClass {
private:
	int TotalArrivals, customersServed, balkingCustomers, maxCars;
	double TotalLitresSold, TotalLitresMissed, TotalWaitingTime,
		TotalServiceTime;

public:
	statsClass ();
	void countArrival () {TotalArrivals += 1;};
	void accumSale (double litres);
	void accumBalk (double litres);
	void accumWaitingTime (double interval) {TotalWaitingTime += interval;};
	void accumServiceTime (double interval) {TotalServiceTime += interval;};
	void snapshot ();
};

statsClass * stats;

statsClass::statsClass ()
{
	TotalArrivals = 0;
	customersServed = 0;
	balkingCustomers = 0;
	maxCars = 0;
	TotalLitresSold = 0.0;
	TotalLitresMissed = 0.0;
	TotalWaitingTime = 0.0;
	TotalServiceTime = 0.0;
}

void statsClass::accumSale (double litres)
{
	customersServed += 1;
	TotalLitresSold += litres;
}

void statsClass::accumBalk (double litres)
{
	balkingCustomers += 1;
	TotalLitresMissed += litres;
}

void statsClass::snapshot ()
{	
	/*
	printf("%8.0f%7i", simulationTime, TotalArrivals);
	printf("%8.3f", carQueue -> emptyTime()/simulationTime);
	*/
	if (TotalArrivals > 0) {
		/*
		printf("%9.3f%8.3f", simulationTime/TotalArrivals,
			(TotalLitresSold + TotalLitresMissed) / TotalArrivals);
			*/
		dataAvgLitre.push_back((TotalLitresSold + TotalLitresMissed) / TotalArrivals);
	}
	/*
	printf ("%7i", balkingCustomers);
	if (customersServed > 0)
		printf ("%9.3f", TotalWaitingTime / customersServed);
	else
		printf ("%9s", "Unknown");
	printf ("%7.3f", TotalServiceTime
		/ (pumpStand -> howManyPumps() * simulationTime));
	printf ("%9.2f", TotalLitresSold * profit
		- cost * pumpStand -> howManyPumps());
	printf ("%7.2f", TotalLitresMissed * profit);

	printf ("%5i\n", carQueue->getMaxCars());
	*/
	TotalArrivals = 0;
	customersServed = 0;
	balkingCustomers = 0;
	maxCars = 0;
	TotalLitresSold = 0.0;
	TotalLitresMissed = 0.0;
	TotalWaitingTime = 0.0;
	TotalServiceTime = 0.0;
}

//-----------------------------------------
// Classes for the specific kinds of events
//-----------------------------------------

class arrivalClass : public eventClass {
private:
	double interarrivalTime();
	BOOLEAN DoesCarBalk (double litres, int queueLength);

public:
	arrivalClass (double T) : eventClass (T) {};
	void makeItHappen ();
};

double arrivalClass::interarrivalTime ()
// The interarrival time of the next car is exponentially distributed.
{
	const int MEAN = 50;
	// seconds, because r is a rate in 1/sec, and this is 1/r
	return - MEAN * log (arrivalStream -> nextrand());
}

BOOLEAN arrivalClass::DoesCarBalk (double litres, int queueLength)
// Deciding whether to balk is an activity that forms part of the
// arrival event, so this procedure belongs among the event routines.
//
// The probability that a car leaves without buying gas (i.e., balks)
// grows larger as the queue length gets larger, and grows smaller
// when the car requries a greater number of litres of gas, such that:
// (1) there is no balking if the queueLength is zero, and
// (2) otherwise, the probability of *not* balking is
//       (40 + litres)/(25 * (3 + queueLength))
{
	return queueLength > 0 && balkingStream -> nextrand()
		> (40 + litres) / (25 * (3 + queueLength));
}

class departureClass : public eventClass {
private:
	pumpClass * whichPump;

public:
	departureClass (double T) : eventClass (T) {};
	void setPump (pumpClass * p) {whichPump = p;};
	void makeItHappen ();
};

class reportClass : public eventClass {
private:
	double intervalToNext; // time between regular reports

public:
	reportClass (double T) : eventClass (T) {};
	void setInterval (double interval) {intervalToNext = interval;};
	void makeItHappen ();
};

void reportClass::makeItHappen ()
{
	stats -> snapshot ();

	// schedule the next interim report.
	setTime (simulationTime + intervalToNext);
	eventList -> insert (this);
}
 
class allDoneClass : public eventClass {
public:
	allDoneClass (double T) : eventClass (T) {};
	void makeItHappen () {stats -> snapshot (); simulating = FALSE;};
};

//---------------
// Event routines
//---------------
    
void arrivalClass::makeItHappen ()
// The car arrival event routine
{
	// Create and initialize a new auto record
	carClass * arrivingCar = new carClass;
	stats -> countArrival ();
	const double L = arrivingCar -> litresNeeded();
	if (DoesCarBalk (L, carQueue->queueSize())) {
		stats -> accumBalk (L);
		delete arrivingCar;
	}
	else {
		arrivingCar -> setTime (simulationTime);
		if (pumpStand -> existsAvailablePump()) {
			pumpClass * Pump = pumpStand -> getAvailablePump ();
			Pump -> startService (arrivingCar);
		}
		else
			carQueue -> insert (arrivingCar);
	}

	// schedule the next arrival, reusing the current event object
	setTime (simulationTime + interarrivalTime());
	eventList -> insert (this);
}

void departureClass::makeItHappen ()
// The departure event routine
{
	// pre whichPump not= NULL and whichPump -> carInService not= NULL

	// Identify the departing car and collect statistics.
	carClass * departingCar = whichPump -> carInService();
	stats -> accumSale (departingCar -> litresNeeded());

	// The car vanishes and the pump is free; can we serve another car?
	delete departingCar;
	if (carQueue->queueSize() > 0)
		whichPump -> startService (carQueue -> getNext());
	else
		pumpStand -> releasePump (whichPump);

	// This departure event is all done.
	delete this;
}

void pumpClass::startService (carClass * Car)
// The start-of-service event routine:
// Connect the Car to this pump, and determine when the service will stop.
{
	// pre pumpStand -> existsAvailablePump();

	// Match the auto to an available pump
	localCarInService = Car;
	const double pumpTime = serviceTime ();


	// Collect statistics
	stats -> accumWaitingTime (simulationTime
		- localCarInService -> arrivalTime());
	stats -> accumServiceTime (pumpTime);

	// Schedule departure of car from this pump
	departureClass * Dep = new departureClass (simulationTime + pumpTime);
	Dep -> setPump (this);
	eventList -> insert (Dep);
}

int main()
{
	//---------------
	// Initialization
	//---------------
	bool simDone = TRUE;
	int iterCnt = 0;
	simulationTime = 0.0;
	simulating = TRUE;

	double ReportInterval, endingTime, confInterval;
	int numBatches;
	cout << "Enter reporting interval: ";
	cin >> ReportInterval;
	cout << "Enter number of Batches: ";
	cin >> numBatches;

	endingTime = ReportInterval * numBatches;
	/*
	cout << "Enter a confidence interval (eg. .95): ";
	cin >> confInterval;
	*/
	int numPumps;
	cout << "Enter number of pumps: ";
	cin >> numPumps;
	cout << "This simulation run uses " << numPumps << " pumps ";
	pumpStand = new pumpStandClass (numPumps);

	// Initialize the random-number streams.

	cout << "and the following random number seeds:\n";
	unsigned short seed;
	cin >> seed;
	cout << "         " << seed;
	arrivalStream = new randStream (seed);
	cin >> seed;
	cout << "         " << seed;
	litreStream = new randStream (seed);
	cin >> seed;
	cout << "         " << seed;
	balkingStream = new randStream (seed);
	cin >> seed;
	cout << "         " << seed;
	serviceStream = new randStream (seed);
	cout << "\n";

	while(simDone){
		// Create and initialize the event queue, the car queue,
		// and the statistical variables.

		eventList = new eventListClass;
		carQueue = new carQueueClass;
		stats = new statsClass;

		// Schedule the end-of-simulation and the first progress report.

		allDoneClass * lastEvent = new allDoneClass (endingTime);
		eventList -> insert (lastEvent);
		if (ReportInterval <= endingTime) {
			reportClass * nextReport = new reportClass (ReportInterval);
			nextReport -> setInterval (ReportInterval);
			eventList -> insert (nextReport);
		}

		// Schedule the first car to arrive at the start of the simulation

		arrivalClass * nextArrival = new arrivalClass (0);
			// Is 0 really the time for the first arrival?
		eventList -> insert (nextArrival);
		/*
		// Print column headings for periodic progress reports and final report

		printf ("%9s%7s%8s%9s%8s%7s%9s%7s%8s%7s%6s\n", " Current", "Total ",
			"NoQueue", "Car->Car", "Average", "Number", "Average", "Pump ",
			"Total", " Lost ", "Max");
		printf ("%9s%7s%8s%9s%8s%7s%9s%7s%8s%7s%6s\n", "   Time ", "Cars ",
			"Fraction", "  Time  ", " Litres ", "Balked", "  Wait ",
			"Usage ", "Profit", "Profit", "Size");
		for (int i = 0; i < 86; i++)
			cout << "-";
		cout << "\n";
		*/
		//------------------------
		// The "clock driver" loop
		//------------------------

		while (simulating) {
			eventClass * currentEvent = eventList -> getNext ();
			simulationTime = currentEvent -> whatTime();
			currentEvent -> makeItHappen();
		}

		//Calculate CI
		confInterval = 1.96;
		double mean1 = 0.0, mean2 = 0.0, dev1 = 0.0, dev2 = 0.0, stderr1, stderr2, CI1, CI2;
		for(int i = 0; i < dataAvgLitre.size()/2; i++)
			mean1 += dataAvgLitre.at(i);
		
		for(int i = dataAvgLitre.size()/2; i < dataAvgLitre.size(); i++)
			mean2 +=dataAvgLitre.at(i);
		
		mean1 = mean1 / (dataAvgLitre.size()/2);
		mean2 = mean2 / (dataAvgLitre.size() - (dataAvgLitre.size()/2));

		for(int i = 0; i < dataAvgLitre.size()/2; i++)
			dev1 += dataAvgLitre.at(i) - mean1;
		
		for(int i = dataAvgLitre.size()/2; i < dataAvgLitre.size(); i++)
			dev2 += dataAvgLitre.at(i) - mean2;
		dev1 = pow(dev1,2);
		dev2 = pow(dev2,2);
		dev1 = sqrt(dev1/(dataAvgLitre.size()/2));
		dev2 = sqrt(dev2/(dataAvgLitre.size()-(dataAvgLitre.size()/2)));

		stderr1 = confInterval * dev1 / sqrt(dataAvgLitre.size()/2);
		stderr2 = confInterval * dev2 / sqrt(dataAvgLitre.size() - (dataAvgLitre.size()/2));
		cout << "Iteration: " << iterCnt << endl;
		cout << "Confidence Level: .95" << endl;
		cout << "CI1: " << mean1 << " +- " << stderr1 << endl;
		cout << "CI2: " << mean2 << " +- " << stderr2 << endl;

		if(abs(mean1 - mean2) < .3){
			simDone = FALSE;
		}

		dataAvgLitre.clear();
		numBatches *= 2;
		simulationTime = 0.0;
		simulating = TRUE;

		iterCnt++;
	}
	return 0;
}

