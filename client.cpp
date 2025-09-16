/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name:
	UIN:
	Date:
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	// assume given patient, time, ecg sensor
	int opt;
	int p = 1; 
	double t = 0.0; 
	int e = 1; 
	// -m (buffer capacity: max num of bytes server can send to client and client sends to server)
	int m = MAX_MESSAGE;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			// arg for server to translate how many bytes are to be transferred 
			case 'm':
				// convert optarg into integer
				m = atoi(optarg);
				break;
		}
	}

	// problem: server runs on separate terminal
	// solution: make client run server using fork and execvp without needing user interference
	// fork(): 
		// duplicates current process
			// parent: orginal process w child's process ID
			// child: copy w PID==0)
			// ie: parent: pid = 1000 -> fork() returns child pid (1001)
	// execvp(): child: pid = 1001 -> fork() returns 0
		// issue: child is still clone of parent (client)
		// solution: 
			// execvp clears child's memory including code stack heap 
			// loads new process (ie. ./server) into empty child
			// child copy is no longer clone of parent (now is server)
	// give args for server ('./server', '-m', '<val for -m arg>', 'NULL' <- lets excevp know done giving args for command)
	// fork
	// in child, run execvp using server args
	// ensure server terminates after client dies by sending QUIT_MSG to server for each open channel
		// call wait to wait for termination

	// problem: currently request sent for data message is only when pt = 1, time = 0, ecg = 1
	// solution: let datamsg accept parameters from getopt function

	// problem: when user provides only p statement, get first 1000 data points of ecg1 & ecg2 and save to file named x1.csv

	// problem: when user provides -f filename, transfer entire file in sections (files are too big to send all at once)

	// problem: when user provides -c, create new private channel of communication between client and server (all requests go thru new pipe)

	// problem: when user provides quit_msg, ensure no open connections at end or temp files in directory
		// follow format that client.cpp does for control channel and apply to private channels if created

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	
	// example data point request
    char buf[MAX_MESSAGE]; // 256 
    datamsg x(1, 0.0, 1); // adjust from hardcode to user's vals
	// send to terminal if user requests one point, else send to file
	
	memcpy(buf, &x, sizeof(datamsg));
	chan.cwrite(buf, sizeof(datamsg)); // send request
	double reply; // allocated space for server's response (one double: ecg value)
	chan.cread(&reply, sizeof(double)); //answer, response
	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	
    // sending a non-sense message, you need to change this
	filemsg fm(0, 0);
	string fname = "teslkansdlkjflasjdf.dat";
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf2, len);  // I want the file length;

	delete[] buf2;
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
}
