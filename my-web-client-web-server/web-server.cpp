#include<algorithm>      //transform find
#include<iostream>
#include<string>
#include<vector>
#include<netdb.h>        //send recv sersockopt
#include<stdio.h>
#include<string.h>       //memset
#include<unistd.h>       //fork close
#include<sys/stat.h>     //fstats
#include<fcntl.h>        //open
#include<sys/sendfile.h> //sendfile
#include "mes-req-res.h"
using namespace std;
#define BACKLOG 5 //Number of pending connections in queue
char* host_name = (char*) "localhost";
char* PORT = (char*) "4000";
string file_dir =  ".";
const string badrequest = "HTTP/1.0 400 Bad Request\r\n" "Content-type: text/html\r\n" "Content-length: 50\r\n" "\r\n" "<html><body><h1>400 Bad Request</h1></body></html>";
const string notfound   = "HTTP/1.0 404 Not Found\r\n" "Content-type: text/html\r\n" "Content-length: 48\r\n" "\r\n";
const string requesttimeout = "HTTP/1.0 408 Request Timeout\r\n" "Content-type: text/html\r\n" "Content-length: 54\r\n" "\r\n"	"<html><body><h1>408 Request Timeout</h1></body></html>";
const string type_gif  = "image/gif";
const string type_html = "text/html";
const string type_jpg  = "image/jpg";
const string type_jpeg = "image/jpeg";
const string type_oct  = "application/octet-stream";
const string type_pdf  = "application/pdf";
const string type_png  = "image/png";
const string type_txt  = "text/plain";
const timeval TIMEOUT{5, 0};
string get_content_type(string filename);
void send_response(int client_sockfd, const http_request& request);
void read_request(int client_sockfd);
int main(int argc, char *argv[])
{
	struct addrinfo hints;//Structs for resolving host names
	struct addrinfo *res_addr;
	struct sockaddr_in client_addr;//Client connection struct / size
	socklen_t client_addr_size = sizeof(client_addr);
	int yes, sockfd;//socket IDs
	if (argc != 1 && argc != 4)//Invalid number of arguments
	{
		cout<<"\nUSAGE:- web-server [hostname] [port] [file-dir]\n\n";
		exit(0);
	}
	if (argc == 4)//Set values if passed in 
	{
		host_name = argv[1];
		PORT = argv[2];
		file_dir = argv[3];
	}
	memset(&hints, 0, sizeof(hints));//Resolve hostname to IP addresss
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host_name, PORT, &hints, &res_addr) != 0) 
	{
		cout<<"\nErrors generated....couldn't resolve hostname to IP address.\n\n";
		return -1;
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);//Creating a socket
	yes = 1;//Allowing concurrent connection
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
	{
		cout<<"\nErrors generated....couldn't set socket options\n\n";
		return -1;
	}
	bind(sockfd, res_addr->ai_addr, res_addr->ai_addrlen);	//Bind address to socket
	if (listen(sockfd, BACKLOG) == -1) //set socket to listen status
	{
		cout<<"\nErrors generated....couldn't set socket to listen\n\n";
		return -1;
	}
	while (1) 
	{
		int client_sockfd = accept(sockfd, (struct sockaddr*) &client_addr, &client_addr_size);//accept a new connection
		if (client_sockfd == -1) 
		{
			cout<<"\nErrors generated....couldn't accept connection";
			return -1;
		}
		if (setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, &TIMEOUT, sizeof(timeval)) == -1) 
		{
			cout<<"\nErrors generated....couldn't set socket options\n\n";
			close(client_sockfd);
			return -1;
		}
		int pid = fork();//Create a child process to handle the client connection
		if (pid < 0) 
		{
			cout<<"\nErrors generated....couldn't create child process for client connection";
			return -1;
		}
		if (pid == 0)//Child 
		{
			cout<<"Connection Established\n";
			close(sockfd);//Child does not need listening socket
			read_request(client_sockfd);//Read the HTTP Request and respond
			close(client_sockfd);//Done with request. Close connection and exit
			exit(0);
		}
		else//Parent 
			close(client_sockfd);//Parent does not need client socket
	}
}
string get_content_type(string filename)
{
	size_t pos = filename.find_last_of('.');
	string type;
	if (pos == string::npos || (pos+1) >= filename.size()) 
		return "";
	type = filename.substr(pos+1);	//Type = everything after last '.'
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);
	if (type == "jpg") 		 return type_jpg;
	else if (type == "jpeg") return type_jpeg;
	else if (type == "gif")  return type_gif;
	else if (type == "html") return type_html;
	else if (type == "pdf")	 return type_pdf;
	else if (type == "png")	 return type_png;
	else if (type == "txt")	 return type_txt;
	else return type_oct;
}
void send_response(int client_sockfd, const http_request& request)
{
	int filefd;
	string filename = request.get_url();
	unsigned int i;	//Throw away everything before the first single slash to get the filename.
	for (i = 0; i < filename.size(); i++) 
		if (filename[i] == '/') 
		{
			if (((i > 0) && filename[i-1] == '/') || ((i < filename.size()-1) && filename[i+1] == '/'))	//Check if there is a slash before or after.
				continue;
			filename = filename.substr(i);
				break;
		}
	if (filename == "/" || i == filename.size())//Default to index.html 
		filename = "/index.html";
	if (filename.back() == '/')	//Throw away extra slash at end of file name. 
		filename = filename.substr(0, filename.size()-1);
	if (file_dir.back() == '/')//Build full file path.Check if file directory already has ending slash 
		filename = file_dir.substr(0, file_dir.size()-1) + filename;//remove extraneous slash
	else filename = file_dir + filename;
	filefd = open(filename.c_str(), O_RDONLY);//Open the file
	if (filefd == -1) 
	{
		send(client_sockfd, notfound.c_str(), notfound.size(), 0);//File not founds
		return;
	}
	struct stat fileStat;//Get file size information
    if (fstat(filefd, &fileStat) < 0) 
    {
       	cout<<"\nErrors generated....couldn't get requested file information: \n"<<filename<<endl;//Could not get file size information.
		send(client_sockfd, notfound.c_str(), notfound.size(), 0);
		return;
    }
	string ok = "HTTP/1.0 200 OK";//Build HTTP response
	string content = "Content-type: " + get_content_type(filename);
	string content_length = "Content-length: " + to_string(fileStat.st_size);
	http_response response;
	response.decode_first_line(ok);
	response.decode_header_line(content);
	response.decode_header_line(content_length);
	vector<char> header = response.encode();//Encode the header as a stream of bytes to send
	send(client_sockfd, &header[0], header.size(), 0);//Send HTTP Response headers
	int total_sent = 0;//Send file
	do 
	{
		int bytes_sent = sendfile(client_sockfd, filefd, NULL, fileStat.st_size);
		if (bytes_sent < 0) 
		{
	    	cout<<"\nErrors generated....couldn't send requested file: \n"<<filename<<endl;//Error while sending.
			send(client_sockfd, notfound.c_str(), notfound.size(), 0);
			return;
		}
		total_sent += bytes_sent;
	} 
	while (total_sent < fileStat.st_size);
	if (total_sent > 0)
		cout<<"Total bytes sent= "<<total_sent<<"\n";
}
void read_request(int client_sockfd)
{
	vector<char> buffer(4096);
	vector<char>::iterator it1, it2;
	int bytes_read = 0;
	int read_location = 0;
	int dist_from_end = 0;
	bool throw_away_next = false;
	http_request request;
	while (1)//Read request into a buffer 
	{
		bytes_read = recv(client_sockfd, &buffer[read_location], buffer.size()-read_location, 0);
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK))//If errno set, then time out 
		{
			send(client_sockfd, requesttimeout.c_str(), requesttimeout.size(), 0);//Timed out
			return;
		}
		if (bytes_read == -1) 
		{
			cout<<"\nErrors generated....couldn't read from socket.\n\n";
			return;
		}
		it1 = find(buffer.begin(), buffer.end(), '\r');//Find end of first line.
		if (it1 == buffer.end())//If couldn't find end of first line, then double buffer size and try to read more. 
		{
			read_location = buffer.size();
			buffer.resize(buffer.size()*2);
		}
		else break;
	}
	if (request.decode_first_line(vector<char>(buffer.begin(), it1)) == -1)//Decode first line. 
	{
		send(client_sockfd, badrequest.c_str(), badrequest.size(), 0);//Bad Request
		return;
	}
	dist_from_end = distance(it1, buffer.end());//Try to move iterator past CRLF to the next header. Check if possible.
	if (dist_from_end < 2) 
	{
        if (dist_from_end == 1) 
            throw_away_next = true;//We only received the \r of the CRLF. Remember to throw away next character (which should be a new line) when read from socket again.
		it1 = buffer.end();
	}
	else advance(it1, 2);//Otherwise advance past "\r\n" to next header.
	while (1)//Decode remaining headers. 
	{
		it2 = find(it1, buffer.end(), '\r');
		if (it1 == it2)//Consecutive CRLFs. End of HTTP Request 
			break;
		if (it2 == buffer.end())//We reached end of buffer, but only received part of the message!
		{
			do 
			{
				read_location = distance(it1, buffer.end());//Get beginning of incomplete header
				copy(it1, buffer.end(), buffer.begin());//Copy remaining incomplete headers to beginning of buffer
				bytes_read = recv(client_sockfd, &buffer[read_location], buffer.size()-read_location, 0);//Read in next section of request
				if ((errno == EAGAIN) || (errno == EWOULDBLOCK))//If errno set, then timeout
				{
					send(client_sockfd, requesttimeout.c_str(), requesttimeout.size(), 0);//Timed out
					return;
				}
				if (bytes_read == -1) 
				{
					cout<<"\nErrors generated....couldn't read from socket.\n\n";
					return;
				}
			} 
			while (bytes_read == 0);
			if (throw_away_next) it1 = buffer.begin()+1;//If throw away next, that means we only got the first half of CRLF previously.Ignore the first character (which should be a \n)
			else it1 = buffer.begin();
			continue;
		}
		else//Decode header line. 
		{
			if (request.decode_header_line(vector<char>(it1, it2)) == -1) 
			{
				send(client_sockfd, badrequest.c_str(), badrequest.size(), 0);//Bad Request
				return;
			}
			dist_from_end = distance(it2, buffer.end());//We have reached the end of the buffer.. but not the end of the request.Set iterator to buffer.end() so that we will read more from socket.
			if (dist_from_end < 2) 
			{
		        if (dist_from_end == 1)
		            throw_away_next = true; //We only received the \r of the CRLF. Remember to throw away next character (which should be a new line) when read from socket again.
		        it1 = buffer.end();
			}
			else it1 = next(it2, 2);//Otherwise advance past "\r\n" to next header.
		}
	}
	send_response(client_sockfd, request);//Respond to the HTTP request
	return;
}