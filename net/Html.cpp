#include "Socket.h"
#include "Error.h"

bool MSocket::ResponseHeader_Send(pConnection cptr, string url)
{
	string head;
	vector<string> body;

	cptr->sendsize = getFileSize(url.c_str());

	if (url == "html/404.html")
	{
		head = cptr->Httprq->headinfo[VERSION] + " 404 " + "Not Found\r\n";
	}
	else
	{
		head = cptr->Httprq->headinfo[VERSION] + " 200 " + "OK\r\n";
	}

	if (url.find(".html") != string::npos)
	{
		HtmlResponse_Get(cptr, body);
		body.push_back("Content-Type: text/html\r\n");
	}
	else if (url.find(".css") != string::npos)
	{
		HtmlResponse_Get(cptr, body);
		body.push_back("Content-Type: text/css\r\n");
	}
	else if (url.find(".jp") != string::npos)
	{
		HtmlResponse_Get(cptr, body);
		body.push_back("Content-Type: image/jpeg\r\n");
	}
	else if (url.find(".gif") != string::npos)
	{
		HtmlResponse_Get(cptr, body);
		body.push_back("Content-Type: image/gif\r\n");
	}
	else if (url.find(".png") != string::npos)
	{
		HtmlResponse_Get(cptr, body);
		body.push_back("Content-Type: image/png\r\n");
	}

	body.push_back("Content-Length: "
		+ to_string(cptr->sendsize) + "\r\n\r\n");

	size_t n = send(cptr->fd, head.c_str(), head.size(), 0);
	if (n <= 0)
	{
		//cerr << "Debug:> Send response head message failed." << endl;
		return false;
	}
	//cerr << head;

	for (int i = 0; i < body.size(); i++)
	{
		n = send(cptr->fd, body[i].c_str(), body[i].size(), 0);
		//cerr << body[i];
		if (n <= 0)
		{
			//cerr << "Debug:> Send response head message failed." << endl;
			return false;
		}
	}

	//cerr << "Debug:> Send response head message success." << endl << endl;
	return true;
}


void MSocket::HtmlResponse_Get(pConnection cptr, vector<string>& body)
{
	bool language = false;

	body.push_back("Date: " + GetResponseTime());
	body.push_back("Server: Dallous_S\r\n");
	body.push_back("Accept-Ranges: bytes\r\n");
	//body.push_back("Allow: GET,POST\r\n");
	body.push_back("Cache-Control: no-cache\r\n");
	//body.push_back("Connection: keep-alive\r\n");

	for (int i = 0; i < cptr->Httprq->body.size(); i++)
	{
		if (cptr->Httprq->body[i].name == "Accept-Language")
		{
			body.push_back("Content-Language: " + cptr->Httprq->body[i].value + "\r\n");
			language = true;
			break;
		}
	}

	if (!language)
	{
		body.push_back("Content-Language: en,zh\r\n");
	}

	return;
}
	