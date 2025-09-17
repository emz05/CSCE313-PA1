/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Emily Ji
	UIN: 834002235
	Date: 09/16/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <algorithm>

using namespace std;


int main (int argc, char *argv[]) {
	// assume given patient, time, ecg sensor
	int opt;
	// initialize to vals that DNE to validate if all parameters exist 
		// true -> send one data pt
		// else -> send first 1000 of patient
	int p = -1; 
	double t = -1; 
	int e = -1; 
	// -m (buffer capacity: max num of bytes server and client can send to each other
	int buffer = MAX_MESSAGE;
	// if called, user wants to create new private channel 
	bool c = false;
	vector<FIFORequestChannel*> channels;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
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
				buffer = atoi(optarg);
				break;
			case 'c':
				c = true;
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

	// implementation:
	std::string buffer_str = std::to_string(buffer);
	char* args[] = {(char*) "./server",(char*) "-m",(char*) buffer_str.c_str(), nullptr};
	pid_t pid = fork();
	// when in child process
	if(pid == 0){
		execvp(args[0], args);
	}

	// problem: when user provides quit_msg, ensure no open connections at end or temp files in directory
		// follow format that client.cpp does for control channel and apply to private channels if created

    FIFORequestChannel control_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&control_chan); 

	// problem: when user provides -c, create new private channel of communication between client and server (all requests go thru new channel)
	// implementation:
	if(c){
		// send new channel request to server
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
    	control_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
		// recieve name from server => var to hold name
		char reply[MAX_MESSAGE];
		// create response from server
		control_chan.cread(reply, sizeof(reply));
		// call FIFORequestChanell constructor with name from server
		FIFORequestChannel* new_chan = new FIFORequestChannel(reply, FIFORequestChannel::CLIENT_SIDE);
		// push back new channel into vector, channels is a vector pointer => provide address for it to point to
		channels.push_back(new_chan);
	}

	// used channel is most recently created if exists
	FIFORequestChannel chan = *(channels.back());
	
	// single data point retrival 
	if(p!= -1 && t != -1 && e != -1){
		char buf[MAX_MESSAGE]; // 256 
		// problem: currently request sent for data message is only when pt = 1, time = 0, ecg = 1
		// solution: let datamsg accept parameters from getopt function
		// implementation: 
		datamsg x(p, t, e); // adjust from hardcode to user's vals
		// send to terminal if user requests one point, else send to file
		// copy bytes at memory address of x into pre allocated space (buf) 
			// only copy num of bytes the struct, datamsg, occupies in memory
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // send request
		double reply; // allocated enough space for server's response as doubles take up 8 bytes (one double: ecg value)
		// place response into address of reply (pointing to location to store)
		chan.cread(&reply, sizeof(double)); //answer, response
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	// problem: when user provides only p statement, get first 1000 data points of ecg1 & ecg2 and save to file named x1.csv
	// solution: 
		// 1000* { 2*{ allocate space for request
		// store request into var
		// copy raw bytes of request from var's address into allocated space
		// write copy of request to server with as much bytes datamsg obj takes
		// allocate space for response
		// read response and transfer into address of allocated space 
		// send response to file name x1.csv in recieved folder }}

	// implementation:
	// 1000 data point retrival
	else if(p != -1 && t == -1 && e == -1){
		std::ofstream outfile("received/x1.csv");
		for(int i =0; i < 1000; ++i){
			char buf[MAX_MESSAGE]; 
			// time moves in .004 increments
			double curr_t=i*.004;
			for(int j =1; j < 3; ++j){
				datamsg x(p, curr_t, j); 
				memcpy(buf, &x, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg)); 
				double reply;
				chan.cread(&reply, sizeof(double)); 
				if(j == 1){
					outfile << reply << ", ";
				}
				else{
					outfile << reply << endl;
				}
			}
		}
		outfile.close();
	}
	
	else if(filename != ""){
		// sending a non-sense message, you need to change this
		filemsg fm(0, 0);
		string fname = filename;

		std::ofstream outfile("received/" + fname, std::ios::binary);

		// problem: when user provides -f filename, transfer entire file into recieved directory (send in sections as files are too big to send all at once)
			// (offset: starting pt, length)
			// account for last transfer where byte size might be different than default buffer capacity
		// solution:
			// 1. send request to server to recieve total byte size of file
			// allocate space for request: (request takes up filemsg struct + filename + 1 <- store into len)
			// copy bytes from filemsg address into allocated space with size of struct
			// copy bytes from filename address into allocated space with size of filesize + 1
			// write bytes in allocated space to server with size of len
			// allocate space for response and store read val as total byte size of file
			// 2. send files in sections, user inputs num of bytes to be sent per section (buffer capacity: m)
			// (filesize / m) + 1 = num of sections
			// for(sections)
				// 
		// implementation:
		
		// +1 accounts for null terminator 
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg)); // copy file message into buffer
		strcpy(buf2 + sizeof(filemsg), fname.c_str()); // copy file name into buffer
		chan.cwrite(buf2, len);  // I want the file length; <- send buffer to server (file request)
		// file request returns file length
		int64_t filesize = 0; // stores how many bytes are in the file
		chan.cread(&filesize, sizeof(int64_t));

		// size of max server response (buffer?)
		char* buf3 = new char[MAX_MESSAGE];
 
		for(int i =0; i <= filesize/buffer; ++i){
			filemsg* file_req = (filemsg*) buf2;
			(*file_req).offset = buffer*i; // starting point
			// problem: cannot request more bytes than needed or else segmentation in server
			// solution: length: min(buffer capacity, total file bytes % buffer capacity: remainder)
			// implementation:
			(*file_req).length = std::min<int64_t>(buffer, filesize - (*file_req).offset); // step size
			chan.cwrite(buf2, len);
			chan.cread(buf3, (*file_req).length);
			// write buf3 into file in recieved directory
			outfile.write(buf3, (*file_req).length);
		}

		delete[] buf2;
		delete[] buf3;
	}

	/*else{
		// unknown message
		MESSAGE_TYPE m = UNKNOWN_MSG;
		chan.cwrite(&m, sizof(MESSAGE_TYPE));
	}*/

	
	
	for(auto curr : channels){
		MESSAGE_TYPE m = QUIT_MSG;
    	(*curr).cwrite(&m, sizeof(MESSAGE_TYPE));
		if(curr != &control_chan){
			delete curr;
		}
	}
	channels.clear();

	// parent cleans up after child
	int status;
	waitpid(pid, &status, 0);
}

	// notes:
	// sample command line to check if files types are same:
		//diff BIMDC/8.csv recieved/8.csv

	// file request = file msg + filename
		// part 1 (file msg): type, offset, length
		// part 2 (filename): filename string ('8.csv')


	// buff req (size of len) = size(filemsg) + size(filename)
	// buff res (size of buffer capacity)