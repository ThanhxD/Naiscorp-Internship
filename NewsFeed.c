#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#define ACCOUNT "email=acount.fool0@gmail.com&pass=abcd_123"
#define HOSTNAME "www.facebook.com"

int Create_socket(char *);
void Send_request(char *, SSL *);
void Reveive_response(char *, SSL *);
void Get_cookie(char *, char *);

int main() {

	SSL *ssl;
	SSL_CTX *ctx;
	const SSL_METHOD *method;
	int sockfd;

	// initializing OpenSSL
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();

	// initialize SSL library and register algorithms
	if (SSL_library_init() < 0) {
		printf("Error: Could not initialize the OpenSSL library!\n");
	}

	// set SSLv2 client helo also announce SSLv3 and TLSv1
	method = SSLv23_client_method();

	// create new SSL context
	ctx = SSL_CTX_new(method);
	if (ctx == NULL) {
		printf("Error: Unable to create new SSL context structure!\n");
	}

	// disabling SSLv2 will leave v3 and TLSv1 fo negotiation
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

	// create new SSL connection state object
	ssl = SSL_new(ctx);

	// create the socket
	sockfd = Create_socket(HOSTNAME);
	if (sockfd != 0) {
		printf("Successfully connect to the socket!\n");
	}

	// attach the SSL session tho the socket descriptor
	SSL_set_fd(ssl, sockfd);

	// try to connect
	if (SSL_connect(ssl) != 1) {
		printf("Error: Could not build a SSL session\n");
	} else {
		printf("Successfully enabled SSL/TLS session\n");
	}

	/************* Send and Receive HTTP messages **********/

	printf("\n==============Start====================\n");
	// first rq for login form
	char request1[] = "HEAD / HTTP/1.1\nHost: www.facebook.com\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:47.0) Gecko/20100101 Firefox/47.0\nConnection: keep-alive\n\n";
	// second for post login info
	char request2_fm[] = "POST /login.php?login_attempt=1&lwv=110 HTTP/1.1\nHost: www.facebook.com\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:47.0) Gecko/20100101 Firefox/47.0\nCookie: %s\nContent-Type: application/x-www-form-urlencoded\nContent-Length: %d\nConnection: keep-alive\n\n%s";
	//third for html content 
	char request3_fm[] = "GET /?stype=lo&jlou=AfckdnKWRpamqRBPiU6zEbGd08BpXCL_meedSJ4vd73GvD12cQ-bI1w0jP6-Jz0TJUvXbPhlr7pxSaLHEqZxTmT-FldiCeOOUqR5RXoXIvLubQ&smuh=54996&lh=Ac_qi94nsLh9UDvh HTTP/1.1\nHost: www.facebook.com\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:47.0) Gecko/20100101 Firefox/47.0\nAccept-Language: en-US,en;q=0.5\nAccept-Encoding: gzip, deflate, br\nCookie: %s\nConnection: keep-alive\n\n";

	char *request2 = NULL, *request3 = NULL, *response = NULL, *cookie = NULL;
	const int RESPONSE_SIZE = 1048576;  // 1MB
	const int COOKIE_SIZE = 102400; 	// 100KB

	response = (char *) malloc(RESPONSE_SIZE*sizeof(char));
	cookie = (char *) malloc(COOKIE_SIZE*sizeof(char));
	
	//request1
	Send_request(request1, ssl);
	memset(response, '\0', RESPONSE_SIZE*sizeof(char));
	Reveive_response(response, ssl);
	printf("===>Response1: \n%s\n", response);
	
	memset(cookie, '\0', COOKIE_SIZE*sizeof(char));
	Get_cookie(cookie, response);
	printf("===>Cookie1: \n%s\n", cookie);
	
	//request2
	request2 = (char *) malloc(strlen(request2_fm) + strlen(ACCOUNT) + strlen(cookie) + 3);
	sprintf(request2, request2_fm, cookie, strlen(ACCOUNT), ACCOUNT);
	//printf("===>Request2: \n%s\n", request2);
	Send_request(request2, ssl);
	memset(response, '\0', RESPONSE_SIZE);
	Reveive_response(response, ssl);
	printf("===>Response2: \n%s\n", response);
	
	memset(cookie, '\0', COOKIE_SIZE);
	Get_cookie(cookie, response);
	printf("===>Cookie2: \n%s\n", cookie);
	Reveive_response(response, ssl);

	//request3
	request3 = (char *) malloc(strlen(request3_fm) + strlen(cookie));
	sprintf(request3, request3_fm, cookie);
	Send_request(request3, ssl);
	memset(response, '\0', RESPONSE_SIZE);
	Reveive_response(response, ssl);
	printf("===>Response3: \n%s\n", response);
	memset(response, '\0', RESPONSE_SIZE);
	Reveive_response(response, ssl);
	printf("===>Response3: \n%s\n", response);
	memset(response, '\0', RESPONSE_SIZE);
	Reveive_response(response, ssl);
	printf("===>Response3: \n%s\n", response);

	free(response);
	free(cookie);
	free(request2);
	free(request3);

	/************************* End *************************/
	printf("\n===============End=====================\n");
	SSL_free(ssl);
	close(sockfd);
	SSL_CTX_free(ctx);

	return 0;
}

int Create_socket(char *hostname) {

	int sockfd;
	int port = 443;
	struct hostent *host;
	struct sockaddr_in dest_addr;

	// get host
	host = gethostbyname(hostname);
	if (host == NULL) {
		printf("Error: Cannot gethostbyname()!\n");
		exit(0);
	}

	// create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// sockaddr struct
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = *(long *)(host->h_addr);

	memset(&(dest_addr.sin_zero), '\0', 8);
	// connect
	if (connect(sockfd, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr)) == -1) {
		printf("Error: Could not connect to host!\n");
	}

	return sockfd;

}

void Send_request(char *request, SSL *ssl) {

	int bytes;						// number of bytes actually written 
	int sent = 0;					// number of bytes sent
	int total = strlen(request);	// total bytes
	do {
		bytes = SSL_write(ssl, request + sent, total - sent);
		if (bytes < 0) {
			printf("Error: Send request\n");
			exit(0);
		}
		if (bytes == 0) break;
		sent += bytes;
		printf("Sent...%d bytes\n", sent);
	} while (bytes > 0);
	printf("Send DONE\n");

}

void Reveive_response(char *resp, SSL *ssl) {

	const int BUFFER_SIZE = 1024;
	char response[1048576];
	char *buffer = NULL;			// to read from ssl
	int bytes;						// number of bytes actually read
	int received = 0;				// number of bytes received

	buffer = (char *) malloc(BUFFER_SIZE*sizeof(char));		// malloc
	memset(response, '\0', sizeof(response));				// response assign = '\0'
	do{
		memset(buffer, '\0', BUFFER_SIZE);					// empty buffer
		bytes = SSL_read(ssl, buffer, BUFFER_SIZE);
		if (bytes < 0) {
			printf("Error: Receive response\n");
			exit(0);
		}
		if (bytes == 0) break;
		received += bytes;
		printf("Received...%d bytes\n", received);
		strncat(response, buffer, bytes);					// concat buffer to response
	} while (SSL_pending(ssl));								// while pending
	response[received] = '\0';
	printf("Receive DONE\n");
	free(buffer);
	strcpy(resp, response);									// return via resp
	
}

void Get_cookie(char *ck, char *message) {

	const int COOKIE_SIZE = 102400;
	int i, j, length = 0;
	char temp[] = "Set-Cookie: ";
	char cookie[COOKIE_SIZE];
	char *start = NULL;
	char *msg = NULL;
	char *st = NULL;
	char *buffer = NULL;

	msg = strstr(message, temp); 						// find set-cookie in message
	if (msg == NULL){									// not found
		strcpy(ck, "null");								// return null
		return;
	}
	start = (char *) malloc(COOKIE_SIZE*sizeof(char));
	buffer = (char *) malloc(COOKIE_SIZE*sizeof(char));
	strcpy(start, msg);									// copy msg to start
	do {
		memset(buffer, '\0', COOKIE_SIZE*sizeof(char));
		st = strstr(start, temp);						// find set-cookie in start
		if (st == NULL) break;							// don't have anymore => break
		st += strlen(temp);								// move st pointer to content of feild
		i = 0;
		while (st[i] != '\n') i++;						// find the character endline '\n'
		//strncpy(buffer, st, i);							// copy content to buffer
		//printf("___%s\n", buffer);
		for (j = 0; j < i; j++) {
			buffer[j] = st[j];
		}
		//printf("__ %s\n", buffer);
		//strncat(cookie, buffer, i);						// concat buffer to cookie
		for (j = 0; j < i; j++) {
			cookie[length+j] = buffer[j];
			//printf("+++%s\n", cookie);
		}
		cookie[length+i] = ';';							// add ;
		length += strlen(buffer) + 1;
		//printf("__%d__\n", length);
		start += (strlen(temp) + i + 1);				// move start pointer to next feild
	} while (st != NULL);
	cookie[length] = '\0';
	free(buffer);
	strcpy(ck, cookie);					// return via ck

}