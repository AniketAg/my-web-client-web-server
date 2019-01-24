#include<algorithm>  //find
#include<iostream>
#include<fstream>
#include<string>
#include<iterator>
#include<netdb.h>    //recv send connect...
#include<stdio.h>
#include<string.h>   //memset
#include<unistd.h>   //close
#include "mes-req-res.h"
using namespace std;
const timeval TIMEOUT{5, 0};
char* host_name;
char* port;
char* file_name;
char* URL;
void getresponse(int sock_id); 
int main(int argc, char *argv[])
{
    int count = argc - 1;
    int number = 1;
    if (argc < 2) 
    {
	    cout<<"USAGE:- web-client [URL]\n";
    	exit(0);
	}
    while (count > 0) 
    {
        struct addrinfo hints;//Structs for resolving host names
        struct addrinfo *res_addr;
        int sock_id;//socket IDs
        string urlstring, host, filename, portname;
        URL = argv[number];
                
        memset(&hints, 0, sizeof(hints));//Resolve hostname to an IP address
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;//Provides sequenced, reliable, two-way, connection-based byte streams.
                                        // Uses the Transmission Control Protocol (TCP) for the Internet address family (AF_INET).
        urlstring = (string) URL;
        portname = "80";
        //Parsing Url
        host = urlstring;
	    host.erase(0, 7);//gets rid of http://
    	filename = host;
    	size_t delimiter = host.find(":");//looking for port number now if included
    	if ((signed) delimiter != -1) 
    	{
    	    portname = host;
    	    host = host.substr(0, delimiter);
    	    portname.erase(0, delimiter + 1);
    	    portname = portname.substr(0, portname.find("/"));
    	}
    	else   host = host.substr(0, host.find("/"));
    	filename.erase(0, filename.find("/"));//looking for file directory here
        //Done Parsing
        host_name = (char*) host.c_str();
        port = (char*) portname.c_str();
        file_name = (char*) filename.c_str();
        if (getaddrinfo(host_name, port, &hints, &res_addr) != 0) 
        {
            cout<<"Error hostname "<<host_name<<"cannot be not resolved to an IP address.\n";
            return -1;
        }
        sock_id = socket(AF_INET, SOCK_STREAM, 0);//Create socket
        if (setsockopt(sock_id, SOL_SOCKET, SO_RCVTIMEO, &TIMEOUT, sizeof(timeval)) == -1)//Set socket options to include timeout after 5 seconds
        {
            cout<<"Errors generated....couldn't set socket options\n";
            close(sock_id);
            return -1;
        }
        if (connect(sock_id, res_addr->ai_addr, res_addr->ai_addrlen) == -1)//Connect socket to server
        {
            cout<<"Errors generated....couldn't connect to server\n";
            return -1;
        }
        string message = "GET " + urlstring + " HTTP/1.0\r\n" "Host: " + host + "\r\n" "Connection: close\r\n" "User-Agent: Wget/1.15 (linux-gnu)\r\n" "Accept: */*\r\n" "\r\n";
        send(sock_id, message.c_str(), message.size(), 0);
        getresponse(sock_id);
        close(sock_id);
        count--;number++;
    }
    return 0;
}
void getresponse(int sock_id) 
{
    vector<char> buffer(4096);
    vector<char>::iterator it1, it2;
    long bytes_read = 0;
    int read_location = 0;
    int dist_from_end = 0;
    int totalbytes_read = 0;
    bool throw_away_next = false;
    http_response response;

    while (1)//Read request into a buffer 
    {
        bytes_read = recv(sock_id, &buffer[read_location], buffer.size()-read_location, 0);
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||(errno ==  EINPROGRESS))//If errno set, then time out 
        {
            cout<<"Errors generated....The response timed out.\n";
            return;
        }
        if (bytes_read == -1) 
        {
            cout<<"Errors generated....couldn't read from socket.\n";
            return;
        }
        totalbytes_read += bytes_read;

        it1 = find(buffer.begin(), buffer.end(), '\r');//Find end of first line.
        if (it1 == buffer.end())//If could not find end of first line, then double buffer size and try to read more. 
        {
            read_location = buffer.size();
            buffer.resize(buffer.size()*2);
        }
        else  break;
    }
    response.decode_first_line(vector<char>(buffer.begin(), it1));
    dist_from_end = distance(it1, buffer.end());//Try to move iterator past CRLF to the next header. Check if possible.

    if (dist_from_end < 2) 
    {
        if (dist_from_end == 1) 
            throw_away_next = true;//We only received the \r of the CRLF. Remember to throw away next character (which should be a new line) when read from socket again.
        it1 = buffer.end();
    }
    else advance(it1, 2);//Otherwise advance past "\r\n" to next header
    while (1)//Decode remaining headers. 
    {
        it2 = find(it1, buffer.end(), '\r');
        if (it1 == it2)//Consecutive CRLFs. End of HTTP Request 
        {
            if (response.get_status() == "404")//404 
            { 
                cout<<"Error 404 generated: The document contained in: "<<URL<<" cannot be found.\n";
                return;
            }
            else if (response.get_status() == "400")//400 
            { 
                cout<<"Error 400 generated: Bad request: "<<URL<<endl;//some syntax error(usually)
                return;
            }
            else 
            { 
                string filenamestring(file_name);//200 success, just download file
                if (filenamestring == "/" || filenamestring.size() == 0)//if file is /, change it to index.hmtl
                    file_name = (char*) "index.html";
                else 
                {
                    int pos = filenamestring.find_last_of('/');
                    filenamestring = filenamestring.substr(pos + 1, -1);
                    file_name = (char*) filenamestring.c_str();//make file_name point to current directory
                }
                vector<char> payload = response.get_payload();
                remove(file_name);//Deletes the file with the same name, so the new downloaded one can replace
            }
            ofstream output_file(file_name);//Create output stream to filename specified by file_name
            ostream_iterator<char> it(output_file);//Outputs the contents of the vector to the file
            vector<char> payload(256000);//Now get the payload
            dist_from_end = distance(it1, buffer.end());
            if (dist_from_end < 2) it1 = buffer.end();
            else it1 = next(it1, 2);//go to beginning of the payload
            int bodybytes_read = totalbytes_read - (int) distance(buffer.begin(), it1);
            it2 = next(it1, bodybytes_read);
            it = copy(it1, it2, it);
            do//If the buffer is smaller than the payload size 
            {
                bytes_read = recv(sock_id, &payload[0], payload.size(), 0);//Read in next bytes from socket
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||(errno ==  EINPROGRESS))//If errno set, then time out 
                {
                    cout<<"Errors generated....Response timed out.\n";
                    return;
                }
                if (bytes_read == -1) 
                {
                    cout<<"Errors generated....couldn't read from socket.\n";
                    return;
                }
                it = copy(payload.begin(), payload.begin() + bytes_read, it);//Set it1 to beginning of new buffer
            } 
            while (bytes_read != 0);
            output_file.close();
            return;
        }
        if (it2 == buffer.end())//We reached end of buffer, but only received part of the message! 
        {
            read_location = distance(it1, buffer.end());//Get beginning of incomplete header
            copy(it1, buffer.end(), buffer.begin());//Copy remaining incomplete headers to beginning of buffer
            bytes_read = recv(sock_id, &buffer[read_location], buffer.size()-read_location, 0);//Read in next section of request
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||(errno ==  EINPROGRESS))//If errno set, then time out 
            {
                cout<<"Errors generated....Response timed out.\n";
                return;
            }
            if (bytes_read == -1) 
            {
                cout<<"Errors generated....couldn't read from socket.\n";
                return;
            }
            else if (bytes_read == 0) 
            {
                cout<<"Errors generated....Socket has been closed by the server. The file name contained in: "<<URL<<" may be invalid.\n";
                return;
            }
            totalbytes_read += bytes_read;
            
            if (throw_away_next) it1 = buffer.begin()+1;//If throw away next, that means we only got the first half of CRLF previously. Ignore the first character (which should be a \n)
            else it1 = buffer.begin();
            continue;
        }
        else//Decode header line. 
        {
            response.decode_header_line(vector<char>(it1, it2));
            dist_from_end = distance(it2, buffer.end());//We have reached the end of the buffer.. but not the end of the request. Set iterator to buffer.end() so that we will read more from socket.
            if (dist_from_end < 2) 
            {
                if (dist_from_end == 1) 
                    throw_away_next = true;//We only received the \r of the CRLF. Remember to throw away next character (which should be a new line) when read from socket again.
                it1 = buffer.end();
            }
            else  it1 = next(it2, 2);//Otherwise advance past "\r\n" to next header.
        }
    }
}