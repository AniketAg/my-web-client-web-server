#include "mes-req-res.h"
#include <algorithm>
using namespace std;
//HTTP MESSAGE
vector<char> http_message::encode_header() const
{
	vector<char> headers;
	for(map<string, string>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it) 
	{
		string h = it->first + ": " + it->second + "\r\n";
		headers.insert(headers.end(), h.begin(), h.end());
	}
	headers.push_back('\r');
	headers.push_back('\n');
	return headers;
}
int http_message::decode_header_line(vector<char> line)
{
	vector<char>::iterator it = find(line.begin(), line.end(), ':');//Find the colon.
	if (it == line.end() || it == line.begin())//Invalid syntax. Either no colon, or no key.
		return -1;
	string key(line.begin(), it);//Key = beginning to colon
	if ((distance(it, line.end()) < 2) || (*(next(it)) != ' '))//Have to advance ahead 2 indices. Check will not overrun end. Check there is a space after colon.
		return -1;
	advance(it, 2);
	string value(it, line.end());//Iterator now points to beginning of key
	set_header(key, value);
	return 0;
}
string http_message::get_header(string key) const
{
	transform(key.begin(), key.end(), key.begin(), ::tolower);//Transform to lower case.
	map<string, string>::const_iterator header = m_headers.find(key);
	if (header == m_headers.end())//Header does not exist. Return empty string 
		return "";
	return header->second;
}
void http_message::set_header(string key, string value)
{
	transform(key.begin(), key.end(), key.begin(), ::tolower);//Transform to lower case.
	m_headers[key] = value;
}
int http_message::decode_header_line(string line){return decode_header_line(vector<char>(line.begin(), line.end()));}
void http_message::set_payload(vector<char> payload){	m_payload = payload;}
void http_message::set_version(string version){m_version = version;}
//HTTP REQUEST
int http_request::decode_first_line(vector<char> line)
{
	vector<char>::iterator it1 = find(line.begin(), line.end(), ' ');//Expected syntax of line:<METHOD> <URL> <version>
	vector<char>::iterator it2 = find(next(it1), line.end(), ' ');
	string method;
	if (it1 == line.end() || it2 == line.end())//Invalid syntax. 
		return -1;
	method = string(line.begin(), it1);
	transform(method.begin(), method.end(), method.begin(), ::toupper);
	if (method != "GET") 
		return -1;
	set_method(method);
	set_url(string(next(it1), it2));
	if (++it2 == line.end())//Invalid syntax. No HTTP version
		return -1;
	set_version(string(it2, line.end()));
	return 0;
}
vector<char> http_request::encode() const
{
	vector<char> request;
	vector<char> headers = encode_header();
	string method = get_method();
	string url = get_url();
	string version = get_version();
	string CRLF = "\r\n";
	request.insert(request.end(), method.begin(), method.end());
	request.push_back(' ');
	request.insert(request.end(), url.begin(), url.end());
	request.push_back(' ');
	request.insert(request.end(), version.begin(), version.end());
	request.insert(request.end(), CRLF.begin(), CRLF.end());
	request.insert(request.end(), headers.begin(), headers.end());
	return request;
}
int http_request::decode_first_line(string line){return decode_first_line(vector<char>(line.begin(), line.end()));}
void http_request::set_url(string url){m_url = url;}
void http_request::set_method(string method){m_method = method;}
//HTTP RESPONSE 
int http_response::decode_first_line(vector<char> line)
{
	vector<char>::iterator it1 = find(line.begin(), line.end(), ' ');	//Expected syntax of line:<version> <status code> <status description>
	vector<char>::iterator it2 = find(next(it1), line.end(), ' ');
	if (it1 == line.end() || it2 == line.end())//Invalid syntax.
		return -1;
	set_version(string(line.begin(), it1));
	set_status(string(next(it1), it2));
	if (++it2 == line.end())//Invalid syntax. No HTTP version 
		return -1;
	set_description(string(it2, line.end()));
	return 0;
}
vector<char> http_response::encode() const
{
	vector<char> response;
	vector<char> headers = encode_header();
	string version = get_version();
	string status = get_status();
	string description = get_description();
	string CRLF = "\r\n";
	response.insert(response.end(), version.begin(), version.end());
	response.push_back(' ');
	response.insert(response.end(), status.begin(), status.end());
	response.push_back(' ');
	response.insert(response.end(), description.begin(), description.end());
	response.insert(response.end(), CRLF.begin(), CRLF.end());
	response.insert(response.end(), headers.begin(), headers.end());
	return response;
}
int http_response::decode_first_line(string line){return decode_first_line(vector<char>(line.begin(), line.end()));}
void http_response::set_status(string status){m_status = status;}
void http_response::set_description(string description){m_description = description;}