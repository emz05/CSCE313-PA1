#ifndef _COMMON_H_
#define _COMMON_H_

#include <iostream>
#include <fstream>

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include <vector>

#define NUM_PERSONS 15  // number of person to collect data for
#define MAX_MESSAGE 256 // maximum buffer size for each message

typedef char byte_t;


// different types of messages: 
// unknown: message type is not found/invalid
// data: get specific data point
// file: get entire patient file
// newchannel: get private server for client (private communication)
// quit: terminate communication
enum MESSAGE_TYPE {UNKNOWN_MSG, DATA_MSG, FILE_MSG, NEWCHANNEL_MSG, QUIT_MSG};


// message requesting a data point
class datamsg {
public:
    MESSAGE_TYPE mtype; // data retrival
    int person; // which patient
    double seconds; // at what time
    int ecgno; // which heart sensor (1 | 2)

    datamsg (int _person, double _seconds, int _eno) {
        mtype = DATA_MSG; // lets server know request type is single data point
        person = _person;
        seconds = _seconds;
        ecgno = _eno;
    }
};


// message requesting a file
class filemsg {
public:
    MESSAGE_TYPE mtype; // file retrival
    __int64_t offset; // start reading from byte X
    int length; // read this many bytes starting from offset
	    
    filemsg (__int64_t _offset, int _length) {
        mtype = FILE_MSG; 
        offset = _offset; 
        length = _length;
    }
};

void EXITONERROR (std::string msg);
std::vector<std::string> split (std::string line, char separator);
__int64_t get_file_size (std::string filename);

#endif
