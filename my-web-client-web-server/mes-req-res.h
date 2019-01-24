#ifndef HTTP_H
#define HTTP_H
#include <vector>
#include <string>
#include <map>
using namespace std;
class http_message 
{
	string m_version;
	map<string, string> m_headers;
	vector<char> m_payload;
public:
	virtual int decode_first_line(vector<char> line) = 0;// Abstract function to decode first line of HTTP Message.
	virtual int decode_first_line(string line) = 0;
	virtual vector<char> encode() const = 0;// Abstract function to encode the HTTP Message.
	vector<char> encode_header() const;//Encodes the HTTP Message headers as a vector of chars.
	int decode_header_line(vector<char> line);//Given a vector of bytes, decode the header line Returns 0 if success. -1 if error.
	int decode_header_line(string line);//Given a string, decode the header line Returns 0 if success. -1 if error.
	void set_header(string key, string value);//Sets a header
	void set_payload(vector<char> payload);//Given a vector of bytes, set the payload
	void set_version(string version);//Sets the HTTP version
	string get_version() const { return m_version; };//	Returns the HTTP version. Either HTTP/1.0 or HTTP/1.1
	vector<char> get_payload() const { return m_payload; };//Returns the payload as a vector of bytes
	string get_header(string key) const;//Returns the value of the header specified by the key
};
class http_request : public http_message 
{
	string m_url;
	string m_method;
public:
	virtual int decode_first_line(vector<char> line);//Decodes the first line of the HTTP request Returns 0 if success. -1 if error.
	virtual int decode_first_line(string line);//Decodes the first line of the HTTP request (given string) Returns 0 if success. -1 if error.
	virtual vector<char> encode() const;////Encode the HTTP Request as a vector of chars.
	void set_url(string url);//Sets the URL of the HTTP Request
	void set_method(string method);//Sets the method of the HTTP Request
	string get_url() const { return m_url; };//Returns the URL of the HTTP Request
	string get_method() const { return m_method; };//Returns the method of the HTTP Request
};
class http_response : public http_message 
{
	string m_status;
	string m_description;
public:
	virtual int decode_first_line(vector<char> line);//Decodes the first line of the HTTP response Returns 0 if success. -1 if error.
	virtual int decode_first_line(string line);//Decodes the first line of the HTTP request (given string) Returns 0 if success. -1 if error.
	virtual vector<char> encode() const;// Encode the HTTP Response as a vector of chars.
	void set_status(string status);//Sets the status of the HTTP response
	void set_description(string description);//Sets the status description of the HTTP response
	string get_status() const { return m_status; };//Returns the status of the HTTP response
	string get_description() const { return m_description; };//Returns the status description of the HTTP response
};
#endif