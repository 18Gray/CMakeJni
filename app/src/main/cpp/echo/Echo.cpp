#include "com_example_lutao_cmakejni_EchoClientActivity.h"
#include "com_example_lutao_cmakejni_EchoServerActivity.h"
#include "com_example_lutao_cmakejni_LocalEchoActivity.h"

// JNI
#include <jni.h>

// NULL
#include <stdio.h>

// va_list, vsnprintf
#include <stdarg.h>

// errno
#include <errno.h>

// strerror_r, memset
#include <string.h>

// socket, bind, getsockname, listen, accept, recv, send, connect
#include <sys/types.h>
#include <sys/socket.h>

// sockaddr_un
#include <sys/un.h>

// htons, sockaddr_in
#include <netinet/in.h>

// inet_ntop
#include <arpa/inet.h>

// close, unlink
#include <unistd.h>

// offsetof
#include <stddef.h>

// Max log message length
#define MAX_LOG_MESSAGE_LENGTH 256

// Max data buffer size
#define MAX_BUFFER_SIZE 80

/**
 * Logs the given message to the application.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param format message format and arguments.
 */
static void LogMessage(
		JNIEnv* env,
		jobject obj,
		const char* format,
		...)
{
	// Cached log method ID
	static jmethodID methodID = NULL;

	// If method ID is not cached
	if (NULL == methodID)
	{
		// Get class from object
		jclass clazz = env->GetObjectClass(obj);

		// Get the method ID for the given method
		methodID = env->GetMethodID(clazz, "logMessage",
				"(Ljava/lang/String;)V");

		// Release the class reference
		env->DeleteLocalRef(clazz);
	}

	// If method is found
	if (NULL != methodID)
	{
		// Format the log message
		char buffer[MAX_LOG_MESSAGE_LENGTH];

		va_list ap;
		va_start(ap, format);
		vsnprintf(buffer, MAX_LOG_MESSAGE_LENGTH, format, ap);
		va_end(ap);

		// Convert the buffer to a Java string
		jstring message = env->NewStringUTF(buffer);

		// If string is properly constructed
		if (NULL != message)
		{
			// Log message
			env->CallVoidMethod(obj, methodID, message);

			// Release the message reference
			env->DeleteLocalRef(message);
		}
	}
}

/**
 * Throws a new exception using the given exception class
 * and exception message.
 *
 * @param env JNIEnv interface.
 * @param className class name.
 * @param message exception message.
 */
static void ThrowException(
		JNIEnv* env,
		const char* className,
		const char* message)
{
	// Get the exception class
	jclass clazz = env->FindClass(className);

	// If exception class is found
	if (NULL != clazz)
	{
		// Throw exception
		env->ThrowNew(clazz, message);

		// Release local class reference
		env->DeleteLocalRef(clazz);
	}
}

/**
 * Throws a new exception using the given exception class
 * and error message based on the error number.
 *
 * @param env JNIEnv interface.
 * @param className class name.
 * @param errnum error number.
 */
static void ThrowErrnoException(
		JNIEnv* env,
		const char* className,
		int errnum)
{
	char buffer[MAX_LOG_MESSAGE_LENGTH];

	// Get message for the error number
	if (-1 == strerror_r(errnum, buffer, MAX_LOG_MESSAGE_LENGTH))
	{
		strerror_r(errno, buffer, MAX_LOG_MESSAGE_LENGTH);
	}

	// Throw exception
	ThrowException(env, className, buffer);
}

/**
 * Constructs a new TCP socket.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @return socket descriptor.
 * @throws IOException
 */
static int NewTcpSocket(JNIEnv* env, jobject obj)
{
	// Construct socket
	LogMessage(env, obj, "Constructing a new TCP socket...");
	int tcpSocket = socket(PF_INET, SOCK_STREAM, 0);

	// Check if socket is properly constructed
	if (-1 == tcpSocket)
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}

	return tcpSocket;
}

/**
 * Binds socket to a port number.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @param port port number or zero for random port.
 * @throws IOException
 */
static void BindSocketToPort(
		JNIEnv* env,
		jobject obj,
		int sd,
		unsigned short port)
{
	struct sockaddr_in address;

	// Address to bind socket
	memset(&address, 0, sizeof(address));
	address.sin_family = PF_INET;

	// Bind to all addresses
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	// Convert port to network byte order
	address.sin_port = htons(port);

	// Bind socket
	LogMessage(env, obj, "Binding to port %hu.", port);
	if (-1 == bind(sd, (struct sockaddr*) &address, sizeof(address)))
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
}

/**
 * Gets the port number socket is currently binded.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @return port number.
 * @throws IOException
 */
static unsigned short GetSocketPort(
		JNIEnv* env,
		jobject obj,
		int sd)
{
	unsigned short port = 0;

	struct sockaddr_in address;
	socklen_t addressLength = sizeof(address);

	// Get the socket address
	if (-1 == getsockname(sd, (struct sockaddr*) &address, &addressLength))
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
	else
	{
		// Convert port to host byte order
		port = ntohs(address.sin_port);

		LogMessage(env, obj, "Binded to random port %hu.", port);
	}

	return port;
}

/**
 * Listens on given socket with the given backlog for
 * pending connections. When the backlog is full, the
 * new connections will be rejected.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @param backlog backlog size.
 * @throws IOException
 */
static void ListenOnSocket(
		JNIEnv* env,
		jobject obj,
		int sd,
		int backlog)
{
	// Listen on socket with the given backlog
	LogMessage(env, obj,
			"Listening on socket with a backlog of %d pending connections.",
			backlog);

	if (-1 == listen(sd, backlog))
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
}

/**
 * Logs the IP address and the port number from the
 * given address.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param message message text.
 * @param address adress instance.
 * @throws IOException
 */
static void LogAddress(
		JNIEnv* env,
		jobject obj,
		const char* message,
		const struct sockaddr_in* address)
{
	char ip[INET_ADDRSTRLEN];

	// Convert the IP address to string
	if (NULL == inet_ntop(PF_INET,
			&(address->sin_addr),
			ip,
			INET_ADDRSTRLEN))
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
	else
	{
		// Convert port to host byte order
		unsigned short port = ntohs(address->sin_port);

		// Log address
		LogMessage(env, obj, "%s %s:%hu.", message, ip, port);
	}
}

/**
 * Blocks and waits for incoming client connections on the
 * given socket.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @return client socket.
 * @throws IOException
 */
static int AcceptOnSocket(
		JNIEnv* env,
		jobject obj,
		int sd)
{
	struct sockaddr_in address;
	socklen_t addressLength = sizeof(address);

	// Blocks and waits for an incoming client connection
	// and accepts it
	LogMessage(env, obj, "Waiting for a client connection...");

	int clientSocket = accept(sd,
			(struct sockaddr*) &address,
			&addressLength);

	// If client socket is not valid
	if (-1 == clientSocket)
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
	else
	{
		// Log address
		LogAddress(env, obj, "Client connection from ", &address);
	}

	return clientSocket;
}

/**
 * Block and receive data from the socket into the buffer.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @param buffer data buffer.
 * @param bufferSize buffer size.
 * @return receive size.
 * @throws IOException
 */
static ssize_t ReceiveFromSocket(
		JNIEnv* env,
		jobject obj,
		int sd,
		char* buffer,
		size_t bufferSize)
{
	// Block and receive data from the socket into the buffer
	LogMessage(env, obj, "Receiving from the socket...");
	ssize_t recvSize = recv(sd, buffer, bufferSize - 1, 0);

	// If receive is failed
	if (-1 == recvSize)
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
	else
	{
		// NULL terminate the buffer to make it a string
		buffer[recvSize] = NULL;

		// If data is received
		if (recvSize > 0)
		{
			LogMessage(env, obj, "Received %d bytes: %s", recvSize, buffer);
		}
		else
		{
			LogMessage(env, obj, "Client disconnected.");
		}
	}

	return recvSize;
}

/**
 * Send data buffer to the socket.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @param buffer data buffer.
 * @param bufferSize buffer size.
 * @return sent size.
 * @throws IOException
 */
static ssize_t SendToSocket(
		JNIEnv* env,
		jobject obj,
		int sd,
		const char* buffer,
		size_t bufferSize)
{
	// Send data buffer to the socket
	LogMessage(env, obj, "Sending to the socket...");
	ssize_t sentSize = send(sd, buffer, bufferSize, 0);

	// If send is failed
	if (-1 == sentSize)
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
	else
	{
		if (sentSize > 0)
		{
			LogMessage(env, obj, "Sent %d bytes: %s", sentSize, buffer);
		}
		else
		{
			LogMessage(env, obj, "Client disconnected.");
		}
	}

	return sentSize;
}

/**
 * Connects to given IP address and given port.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @param ip IP address.
 * @param port port number.
 * @throws IOException
 */
static void ConnectToAddress(
		JNIEnv* env,
		jobject obj,
		int sd,
		const char* ip,
		unsigned short port)
{
	// Connecting to given IP address and given port
	LogMessage(env, obj, "Connecting to %s:%uh...", ip, port);

	struct sockaddr_in address;

	memset(&address, 0, sizeof(address));
	address.sin_family = PF_INET;

	// Convert IP address string to Internet address
	if (0 == inet_aton(ip, &(address.sin_addr)))
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
	else
	{
		// Convert port to network byte order
		address.sin_port = htons(port);

		// Connect to address
		if (-1 == connect(sd, (const sockaddr*) &address, sizeof(address)))
		{
			// Throw an exception with error number
			ThrowErrnoException(env, "java/io/IOException", errno);
		}
		else
		{
			LogMessage(env, obj, "Connected.");
		}
	}
}

JNIEXPORT void JNICALL Java_com_example_lutao_cmakejni_EchoClientActivity_nativeStartTcpClient
		(JNIEnv* env,
		jobject obj,
		jstring ip,
		jint port,
		jstring message)
{
	// Construct a new TCP socket.
	int clientSocket = NewTcpSocket(env, obj);
	if (NULL == env->ExceptionOccurred())
	{
		// Get IP address as C string
		const char* ipAddress = env->GetStringUTFChars(ip, NULL);
		if (NULL == ipAddress)
			goto exit;

		// Connect to IP address and port
		ConnectToAddress(env, obj, clientSocket, ipAddress,
				(unsigned short) port);

		// Release the IP address
		env->ReleaseStringUTFChars(ip, ipAddress);

		// If connection was successful
		if (NULL != env->ExceptionOccurred())
			goto exit;

		// Get message as C string
		const char* messageText = env->GetStringUTFChars(message, NULL);
		if (NULL == messageText)
			goto exit;

		// Get the message size
		jsize messageSize = env->GetStringUTFLength(message);

		// Send message to socket
		SendToSocket(env, obj, clientSocket, messageText, messageSize);

		// Release the message text
		env->ReleaseStringUTFChars(message, messageText);

		// If send was not successful
		if (NULL != env->ExceptionOccurred())
			goto exit;

		char buffer[MAX_BUFFER_SIZE];

		// Receive from the socket
		ReceiveFromSocket(env, obj, clientSocket, buffer, MAX_BUFFER_SIZE);
	}

exit:
	if (clientSocket > 0)
	{
		close(clientSocket);
	}
}

JNIEXPORT void JNICALL Java_com_example_lutao_cmakejni_EchoServerActivity_nativeStartTcpServer
		(JNIEnv* env,
		jobject obj,
		jint port)
{
	// Construct a new TCP socket.
	int serverSocket = NewTcpSocket(env, obj);
	if (NULL == env->ExceptionOccurred())
	{
		// Bind socket to a port number
		BindSocketToPort(env, obj, serverSocket, (unsigned short) port);
		if (NULL != env->ExceptionOccurred())
			goto exit;

		// If random port number is requested
		if (0 == port)
		{
			// Get the port number socket is currently binded
			GetSocketPort(env, obj, serverSocket);
			if (NULL != env->ExceptionOccurred())
				goto exit;
		}

		// Listen on socket with a backlog of 4 pending connections
		ListenOnSocket(env, obj, serverSocket, 4);
		if (NULL != env->ExceptionOccurred())
			goto exit;

		// Accept a client connection on socket
		int clientSocket = AcceptOnSocket(env, obj, serverSocket);
		if (NULL != env->ExceptionOccurred())
			goto exit;

		char buffer[MAX_BUFFER_SIZE];
		ssize_t recvSize;
		ssize_t sentSize;

		// Receive and send back the data
		while (1)
		{
			// Receive from the socket
			recvSize = ReceiveFromSocket(env, obj, clientSocket,
					buffer, MAX_BUFFER_SIZE);

			if ((0 == recvSize) || (NULL != env->ExceptionOccurred()))
				break;

			// Send to the socket
			sentSize = SendToSocket(env, obj, clientSocket,
					buffer, (size_t) recvSize);

			if ((0 == sentSize) || (NULL != env->ExceptionOccurred()))
				break;
		}

		// Close the client socket
		close(clientSocket);
	}

exit:
	if (serverSocket > 0)
	{
		close(serverSocket);
	}
}

/**
 * Constructs a new UDP socket.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @return socket descriptor.
 * @throws IOException
 */
static int NewUdpSocket(JNIEnv* env, jobject obj)
{
	// Construct socket
	LogMessage(env, obj, "Constructing a new UDP socket...");
	int udpSocket = socket(PF_INET, SOCK_DGRAM, 0);

	// Check if socket is properly constructed
	if (-1 == udpSocket)
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}

	return udpSocket;
}

/**
 * Block and receive datagram from the socket into
 * the buffer, and populate the client address.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @param address client address.
 * @param buffer data buffer.
 * @param bufferSize buffer size.
 * @return receive size.
 * @throws IOException
 */
static ssize_t ReceiveDatagramFromSocket(
		JNIEnv* env,
		jobject obj,
		int sd,
		struct sockaddr_in* address,
		char* buffer,
		size_t bufferSize)
{
	socklen_t addressLength = sizeof(struct sockaddr_in);

	// Receive datagram from socket
	LogMessage(env, obj, "Receiving from the socket...");
	ssize_t recvSize = recvfrom(sd, buffer, bufferSize, 0,
			(struct sockaddr*) address,
			&addressLength);

	// If receive is failed
	if (-1 == recvSize)
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
	else
	{
		// Log address
		LogAddress(env, obj, "Received from", address);

		// NULL terminate the buffer to make it a string
		buffer[recvSize] = NULL;

		// If data is received
		if (recvSize > 0)
		{
			LogMessage(env, obj, "Received %d bytes: %s", recvSize, buffer);
		}
	}

	return recvSize;
}

/**
 * Sends datagram to the given address using the given socket.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @param address remote address.
 * @param buffer data buffer.
 * @param bufferSize buffer size.
 * @return sent size.
 * @throws IOException
 */
static ssize_t SendDatagramToSocket(
		JNIEnv* env,
		jobject obj,
		int sd,
		const struct sockaddr_in* address,
		const char* buffer,
		size_t bufferSize)
{
	// Send data buffer to the socket
	LogAddress(env, obj, "Sending to", address);
	ssize_t sentSize = sendto(sd, buffer, bufferSize, 0,
			(const sockaddr*) address,
			sizeof(struct sockaddr_in));

	// If send is failed
	if (-1 == sentSize)
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}
	else if (sentSize > 0)
	{
		LogMessage(env, obj, "Sent %d bytes: %s", sentSize, buffer);
	}

	return sentSize;
}

JNIEXPORT void JNICALL Java_com_example_lutao_cmakejni_EchoClientActivity_nativeStartUdpClient
        (JNIEnv* env,
		jobject obj,
		jstring ip,
		jint port,
		jstring message)
{
	// Construct a new UDP socket.
	int clientSocket = NewUdpSocket(env, obj);
	if (NULL == env->ExceptionOccurred())
	{
		struct sockaddr_in address;

		memset(&address, 0, sizeof(address));
		address.sin_family = PF_INET;

		// Get IP address as C string
		const char* ipAddress = env->GetStringUTFChars(ip, NULL);
		if (NULL == ipAddress)
			goto exit;

		// Convert IP address string to Internet address
		int result = inet_aton(ipAddress, &(address.sin_addr));

		// Release the IP address
		env->ReleaseStringUTFChars(ip, ipAddress);

		// If conversion is failed
		if (0 == result)
		{
			// Throw an exception with error number
			ThrowErrnoException(env, "java/io/IOException", errno);
			goto exit;
		}

		// Convert port to network byte order
		address.sin_port = htons(port);

		// Get message as C string
		const char* messageText = env->GetStringUTFChars(message, NULL);
		if (NULL == messageText)
			goto exit;

		// Get the message size
		jsize messageSize = env->GetStringUTFLength(message);

		// Send message to socket
		SendDatagramToSocket(env, obj, clientSocket, &address,
				messageText, messageSize);

		// Release the message text
		env->ReleaseStringUTFChars(message, messageText);

		// If send was not successful
		if (NULL != env->ExceptionOccurred())
			goto exit;

		char buffer[MAX_BUFFER_SIZE];

		// Clear address
		memset(&address, 0, sizeof(address));

		// Receive from the socket
		ReceiveDatagramFromSocket(env, obj, clientSocket, &address,
				buffer, MAX_BUFFER_SIZE);
	}

exit:
	if (clientSocket > 0)
	{
		close(clientSocket);
	}
}

JNIEXPORT void JNICALL Java_com_example_lutao_cmakejni_EchoServerActivity_nativeStartUdpServer
		(JNIEnv* env,
		jobject obj,
		jint port)
{
	// Construct a new UDP socket.
	int serverSocket = NewUdpSocket(env, obj);
	if (NULL == env->ExceptionOccurred())
	{
		// Bind socket to a port number
		BindSocketToPort(env, obj, serverSocket, (unsigned short) port);
		if (NULL != env->ExceptionOccurred())
			goto exit;

		// If random port number is requested
		if (0 == port)
		{
			// Get the port number socket is currently binded
			GetSocketPort(env, obj, serverSocket);
			if (NULL != env->ExceptionOccurred())
				goto exit;
		}

		// Client address
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));

		char buffer[MAX_BUFFER_SIZE];
		ssize_t recvSize;
		ssize_t sentSize;

		// Receive from the socket
		recvSize = ReceiveDatagramFromSocket(env, obj, serverSocket,
				&address, buffer, MAX_BUFFER_SIZE);

		if ((0 == recvSize) || (NULL != env->ExceptionOccurred()))
			goto exit;

		// Send to the socket
		sentSize = SendDatagramToSocket(env, obj, serverSocket,
				&address, buffer, (size_t) recvSize);
	}

exit:
	if (serverSocket > 0)
	{
		close(serverSocket);
	}
}

/**
 * Constructs a new Local UNIX socket.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @return socket descriptor.
 * @throws IOException
 */
static int NewLocalSocket(JNIEnv* env, jobject obj)
{
	// Construct socket
	LogMessage(env, obj, "Constructing a new Local UNIX socket...");
	int localSocket = socket(PF_LOCAL, SOCK_STREAM, 0);

	// Check if socket is properly constructed
	if (-1 == localSocket)
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}

	return localSocket;
}

/**
 * Binds a local UNIX socket to a name.
 *
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @param name socket name.
 * @throws IOException
 */
static void BindLocalSocketToName(
		JNIEnv* env,
		jobject obj,
		int sd,
		const char* name)
{
	struct sockaddr_un address;

	// Name length
	const size_t nameLength = strlen(name);

	// Path length is initiall equal to name length
	size_t pathLength = nameLength;

	// If name is not starting with a slash it is
	// in the abstract namespace
	bool abstractNamespace = ('/' != name[0]);

	// Abstract namespace requires having the first
	// byte of the path to be the zero byte, update
	// the path length to include the zero byte
	if (abstractNamespace)
	{
		pathLength++;
	}

	// Check the path length
	if (pathLength > sizeof(address.sun_path))
	{
		// Throw an exception with error number
		ThrowException(env, "java/io/IOException", "Name is too big.");
	}
	else
	{
		// Clear the address bytes
		memset(&address, 0, sizeof(address));
		address.sun_family = PF_LOCAL;

		// Socket path
		char* sunPath = address.sun_path;

		// First byte must be zero to use the abstract namespace
		if (abstractNamespace)
		{
			*sunPath++ = NULL;
		}

		// Append the local name
		strcpy(sunPath, name);

		// Address length
		socklen_t addressLength =
				(offsetof(struct sockaddr_un, sun_path))
				+ pathLength;

		// Unlink if the socket name is already binded
		unlink(address.sun_path);

		// Bind socket
		LogMessage(env, obj, "Binding to local name %s%s.",
				(abstractNamespace) ? "(null)" : "",
				name);

		if (-1 == bind(sd, (struct sockaddr*) &address, addressLength))
		{
			// Throw an exception with error number
			ThrowErrnoException(env, "java/io/IOException", errno);
		}
	}
}

/**
 * Blocks and waits for incoming client connections on the
 * given socket.
 * @param env JNIEnv interface.
 * @param obj object instance.
 * @param sd socket descriptor.
 * @return client socket.
 * @throws IOException
 */
static int AcceptOnLocalSocket(
		JNIEnv* env,
		jobject obj,
		int sd)
{
	// Blocks and waits for an incoming client connection
	// and accepts it
	LogMessage(env, obj, "Waiting for a client connection...");
	int clientSocket = accept(sd, NULL, NULL);

	// If client socket is not valid
	if (-1 == clientSocket)
	{
		// Throw an exception with error number
		ThrowErrnoException(env, "java/io/IOException", errno);
	}

	return clientSocket;
}

JNIEXPORT void JNICALL Java_com_example_lutao_cmakejni_LocalEchoActivity_nativeStartLocalServer(
		JNIEnv* env,
		jobject obj,
		jstring name)
{
	// Construct a new local UNIX socket.
	int serverSocket = NewLocalSocket(env, obj);
	if (NULL == env->ExceptionOccurred())
	{
		// Get name as C string
		const char* nameText = env->GetStringUTFChars(name, NULL);
		if (NULL == nameText)
			goto exit;

		// Bind socket to a port number
		BindLocalSocketToName(env, obj, serverSocket, nameText);

		// Release the name text
		env->ReleaseStringUTFChars(name, nameText);

		// If bind is failed
		if (NULL != env->ExceptionOccurred())
			goto exit;

		// Listen on socket with a backlog of 4 pending connections
		ListenOnSocket(env, obj, serverSocket, 4);
		if (NULL != env->ExceptionOccurred())
			goto exit;

		// Accept a client connection on socket
		int clientSocket = AcceptOnLocalSocket(env, obj, serverSocket);
		if (NULL != env->ExceptionOccurred())
			goto exit;

		char buffer[MAX_BUFFER_SIZE];
		ssize_t recvSize;
		ssize_t sentSize;

		// Receive and send back the data
		while (1)
		{
			// Receive from the socket
			recvSize = ReceiveFromSocket(env, obj, clientSocket,
					buffer, MAX_BUFFER_SIZE);

			if ((0 == recvSize) || (NULL != env->ExceptionOccurred()))
				break;

			// Send to the socket
			sentSize = SendToSocket(env, obj, clientSocket,
					buffer, (size_t) recvSize);

			if ((0 == sentSize) || (NULL != env->ExceptionOccurred()))
				break;
		}

		// Close the client socket
		close(clientSocket);
	}

exit:
	if (serverSocket > 0)
	{
		close(serverSocket);
	}
}
