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
#define RESPONSE_SIZE 1048576
#define COOKIE_SIZE 102400
#define ID_LENGTH 15
#define MAX_FRIEND 1000

int Create_socket(char *);
void Connect_socket();
void Login_and_get_response(char *);
void Get_id_friend_list(int *, char [][ID_LENGTH+1], char *);
void Send_request(char *, SSL *);
void Reveive_response(char *, SSL *, int);
void Get_cookie(char *, char *);

FILE *f, *friend;
SSL *ssl;
SSL_CTX *ctx;
int sockfd;

int main() {

	/********************** Start **************************/

	Connect_socket();

	char *response = malloc(RESPONSE_SIZE*sizeof(char));
	Login_and_get_response(response);

	char id[MAX_FRIEND][ID_LENGTH+1];
	int number_of_friends, i = 0;
	Get_id_friend_list(&number_of_friends, id, response);   //return id - an array include ids of friends.
	printf("##### %d friends\n", number_of_friends);
	for ( i = 0; i < number_of_friends; ++i) {
		printf(" %d_%s\n", i+1, id[i]);
	}

	/************************* End *************************/
	printf("\n===============End=====================\n");
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	close(sockfd);
	fclose(f);

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

void Connect_socket() {

	const SSL_METHOD *method;

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

	// attach the SSL session to the socket descriptor
	SSL_set_fd(ssl, sockfd);

	// try to connect
	if (SSL_connect(ssl) != 1) {
		printf("Error: Could not build a SSL session\n");
	} else {
		printf("Successfully enabled SSL/TLS session\n");
	}

}
void Login_and_get_response(char *resp) {
	// first rq for login form
	char request1[] = "HEAD / HTTP/1.1\nHost: www.facebook.com\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:47.0) Gecko/20100101 Firefox/47.0\nConnection: keep-alive\n\n";
	// second for post login info
	char request2_fm[] = "POST /login.php?login_attempt=1&lwv=110 HTTP/1.1\nHost: www.facebook.com\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:47.0) Gecko/20100101 Firefox/47.0\nCookie: %s\nContent-Type: application/x-www-form-urlencoded\nContent-Length: %d\nConnection: keep-alive\n\n%s";
	//third for html content 
	char request3_fm[] = "GET / HTTP/1.0\nHost: www.facebook.com\nUser-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:47.0) Gecko/20100101 Firefox/47.0\nAccept: text/html\nAccept-Language: en-US,en;q=0.5\nCookie: %s\nConnection: keep-alive\n\n";

	char *request2 = NULL, *request3 = NULL, *response = NULL, *cookie = NULL;
	f = fopen("fb.txt", "w");

	response = (char *) malloc(RESPONSE_SIZE*sizeof(char));
	cookie = (char *) malloc(COOKIE_SIZE*sizeof(char));
	
	//request1
	Send_request(request1, ssl);
	memset(response, '\0', RESPONSE_SIZE*sizeof(char));
	Reveive_response(response, ssl, 0);
	//printf("===>Response1: \n%s\n", response);
	
	memset(cookie, '\0', COOKIE_SIZE*sizeof(char));
	Get_cookie(cookie, response);
	//printf("===>Cookie1: \n%s\n", cookie);
	
	//request2
	request2 = (char *) malloc(strlen(request2_fm) + strlen(ACCOUNT) + strlen(cookie) + 3);
	sprintf(request2, request2_fm, cookie, strlen(ACCOUNT), ACCOUNT);
	//printf("===>Request2: \n%s\n", request2);
	Send_request(request2, ssl);
	memset(response, '\0', RESPONSE_SIZE);
	Reveive_response(response, ssl, 0);
	//printf("===>Response2: \n%s\n", response);
	
	memset(cookie, '\0', COOKIE_SIZE);
	Get_cookie(cookie, response);
	//printf("===>Cookie2: \n%s\n", cookie);
	
	//request3
	request3 = (char *) malloc(strlen(request3_fm) + strlen(cookie));
	sprintf(request3, request3_fm, cookie);
	//printf("===>Request3: \n%s\n", request3);
	Send_request(request3, ssl);
	memset(response, '\0', RESPONSE_SIZE);
	Reveive_response(response, ssl, 1);
	//printf("===>Response3: \n%s\n", response);

	strcpy(resp, response);

	free(response);
	free(cookie);
	free(request2);
	free(request3);

}

void Get_id_friend_list(int *number_of_friends, char id[][ID_LENGTH+1], char *response) { 
	// nhận vào thông điệp response, xử lí và trả về mảng id chứa danh sách id trong friendlist và số friends number_of_friends
	char item[ID_LENGTH+1];
	int i, k, index;
	char *initial_chat_friends_list = strstr(response, "InitialChatFriendsList"); // danh sách chat bắt đầu từ InitialChatFriendsList
	char *list = strstr(initial_chat_friends_list, "list");	// danh sách chat bạn bè bắt đầu từ trường list (trước nó là trường chứa group chat)
	char *end_list = strstr(list, "]"); // hết trường list
	end_list[1] = '\0';
	list += 7;
	index = 0;
	i = 0;
	k = 0;
	/* định dạng list:["10000xxxxxxxxxx-y", "10000xxxxxx-y"...]
	mỗi phần tử trường chứa id và theo sau là dấu -  là y = 0 hoặc y = 2
	mỗi id có 2 phần từ tương ứng với từng y, nên chỉ lấy 1 trong 2 phần tử đó
	từng id lưu vào biến item và gán vào mảng khi đọc xong
	*/
	while (i < strlen(list)) {		//duyệt từng kí tự trên list
		if (list[i] != '-') {		// nếu là số bình thường
			item[k++] = list[i++];	// gán vào item
		} else if (list[i+1] == '2') {	// nếu là dấu -, và tiếp sau là số 2
			item[k] = '\0';		// thì ngắt item
			strcpy(id[index], item);// gán vào mảng id
			index++;		// tăng chỉ số mảng id
			i += 5;			// dịch chuyển lên 5 kí tự
			k = 0;			// gán lại chỉ số mảng item = 0
		} else {		// ngược lại:  là dấu - và tiếp sau là số 0
			i += 5;		// bỏ qua, dịch chuyển lên 5 kí tự
			k = 0;		// gán lại chỉ số mảng item
		}
	}
	*number_of_friends = index;	// trả về số lượng friends

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
		printf("Sent...%d bytes\n", bytes);
	} while (bytes > 0);
	printf("##### Send DONE #####\n");

}

void Reveive_response(char *resp, SSL *ssl, int body_required) {

	char header[1048576];
	char body[1048576] = "";
	int bytes;						// number of bytes actually read
	int received = 0;				// total number of bytes received
	int i, line_length;
	char c[1];

	memset(header, '\0', sizeof(header));				// response assign = '\0'

	/***********Try to read byte by byte***********/

	i = 0;
	line_length = 0;								// to check length of each line
	do {
		bytes = SSL_read(ssl, c, 1);				// read 1 byte to c[0]
		if (bytes  <= 0) break;						// read fall or connection closed
		if (c[0] == '\n') {							// if '\n'
			if (line_length == 0) break;			// empty line, so end header
			else line_length = 0;					// else reset for new line
		} else if ( c[0] != '\r') line_length++;	// inc length
		header[i++] = c[0];						// add to response
		received += bytes;
 	} while (1);
	printf("##### Header DONE #####\n");
	/***********************************************/

	/********Then try to read body if needed********/

	char *buf = malloc(1024*sizeof(char));
	if (body_required) {//read body
		do {
			memset(buf, '\0', 1024*sizeof(char));
			bytes = SSL_read(ssl, buf, 1024);
			if (bytes <= 0) break;
			strcat(body, buf);
			received += bytes;
			printf("Received...%d bytes\n", bytes);
		} while (1);
		printf("\n##### Body DONE #####\n");
	}
	free(buf);

	/***********************************************/
	strcpy(resp, header);
	strcat(resp, body);
}

void Get_cookie(char *ck, char *message) {

	int i, j, length = 0;
	char temp[] = "Set-Cookie: ";
	char cookie[COOKIE_SIZE];
	char *start = NULL;
	char *msg = NULL;
	char *st = NULL;

	msg = strstr(message, temp); 						// find set-cookie in message
	if (msg == NULL){									// not found
		strcpy(ck, "null");								// return null
		return;
	}
	start = (char *) malloc(COOKIE_SIZE*sizeof(char));
	strcpy(start, msg);									// copy msg to start
	do {
		st = strstr(start, temp);						// find set-cookie in start
		if (st == NULL) break;							// don't have anymore => break
		st += strlen(temp);								// move st pointer to content of feild
		i = 0;
		while (st[i] != '\n') i++;						// find the character endline '\n'
		for (j = 0; j < i; j++) {						// copy content to cookie
			cookie[length+j] = st[j];
		}
		cookie[length+i-1] = ';';							// add ';'
		length += i;
		start += (strlen(temp) + i + 1);				// move start pointer to next feild
	} while (st != NULL);
	cookie[length] = '\0';
	strcpy(ck, cookie);					// return via ck

}
