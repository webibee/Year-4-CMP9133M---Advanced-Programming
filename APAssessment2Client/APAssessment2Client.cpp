#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <opencv2/opencv.hpp> // Include OpenCV Library

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // Link the Winsock library
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#endif

const int BUFFER_SIZE = 1024; // Global constant variable to declare buffer size of UDP messages

int main(int argc, char** argv) {
    if (argc < 4) {
        // Display examples of how to run program
        std::cout << "--- Usage: ./distributed_img_client <server_IP> <input_file_img.png> <transformation_required>" << std::endl;
        std::cout << "-- ./distributed_img_client 127.0.0.1:12345 input_img.png Resizing <integer_Width> <integer_Height>" << std::endl;
        std::cout << "-- ./distributed_img_client 127.0.0.1:12345 input_img.png Rotation <double_Angle>" << std::endl;
        std::cout << "-- ./distributed_img_client 127.0.0.1:12345 input_img.png Cropping <integer_StartX> <integer_StartY> <integer_Width> <integer_Height>" << std::endl;
        std::cout << "-- ./distributed_img_client 127.0.0.1:12345 input_img.png Flipping <integer_FlipCode> *0:horizontal, 1+:vertical, Negative:both*" << std::endl;
        std::cout << "-- ./distributed_img_client 127.0.0.1:12345 input_img.png ContrastAdjustment <double_Contrast> *E.g) 2:Increase Contrast by 2, 0.5:Decrease Contrast by 0.5*" << std::endl;
        std::cout << "-- ./distributed_img_client 127.0.0.1:12345 input_img.png BrightnessAdjustment <integer_Brightness> *E.g.) 50:Increase Brightness by 50, -100:Decrease Brightness by 100*" << std::endl;
        std::cout << "-- ./distributed_img_client 127.0.0.1:12345 input_img.png Blurring <integer_MatrixDimension> *Only allows 3 or 5 - 3:Matrix of 3x3, 5:Matrix of 5x5*" << std::endl;
        std::cout << "-- ./distributed_img_client 127.0.0.1:12345 input_img.png Sharpening" << std::endl;
        return 0;
    }

    // If program is executed with 4 or more arguments in command line, store the strings of each argument
    std::string serverIP = argv[1];
    std::string inputImg = argv[2];
    std::string transformation = argv[3];

#ifdef _WIN32
    // Initialize Winsock on Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "--- Winsock initialization failed" << std::endl;
        return 1;
    }
#endif

    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t serverAddressLen = sizeof(serverAddr);

    // Create a UDP socket
    if ((clientSocket = socket(AF_INET,
        SOCK_DGRAM, 0)) == -1) {
        perror("--- Socket creation failed"); // SOCK_DATAGRAM utilises UDP 
#ifdef _WIN32
        WSACleanup(); // Cleanup Winsock
#endif
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));

    /* - Split the serverIP the user input at the command line into IP Addressand Port Number through separating them at the ":" symbol
       - If statement checks to see if the user entered the IP Address and Port Number in the correct format, otherwise will display error message*/
    size_t colonPosition = serverIP.find(':');
    std::string IPAddress = "";
    std::string portStr = "";
    int port;
    if (colonPosition != std::string::npos) {
        IPAddress = serverIP.substr(0, colonPosition);
        portStr = serverIP.substr(colonPosition + 1);
        port = std::stoi(portStr);
    }
    else {
        std::cerr << "--- Invalid input format entered for Server IP. Please use <IP:Port>. " << std::endl;
        return 0;
    }
    

    // Connect
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    // Port number
    serverAddr.sin_port = htons(port);      
    // Checks if IP Address entered is valid
    if (inet_pton(AF_INET, IPAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "--- Invalid IP Address entered." << std::endl;
        return 1;
    }
    // Server's IP address
    inet_pton(AF_INET, IPAddress.c_str(), &serverAddr.sin_addr);

    int n;
    socklen_t len = sizeof(serverAddr);;
    
    //  Attempts to open image file given as argument in command line, otherwise raises error message and stops the program
    std::ifstream imageFile(inputImg, std::ios::binary);
    if (!imageFile) {
        std::cerr << "--- Error opening image file" << std::endl;
        return 1;
    }

    //  Attempts to load image file given as argument in command line into an OpenCV Mat, otherwise raises error message and stops the program
    cv::Mat originalImage = cv::imread(inputImg);
    if (originalImage.empty()) {
        std::cerr << "--- Error: Unable to load image";
        return 1;
    }

    // If the user enters a valid IP Address and Port Number, and a valid image which has been opened and loaded by the program into the OpenCV Mat 'originalImage', the program will continue
    // Get the file size
    imageFile.seekg(0, std::ios::end);
    int fileSize = imageFile.tellg();
    imageFile.seekg(0, std::ios::beg);

    // Calculate the number of packets needed
    int numPackets = (fileSize + BUFFER_SIZE - 1) / BUFFER_SIZE;

    // Send the total number of packets to the server
    if (sendto(clientSocket, reinterpret_cast<const char*>(&numPackets), sizeof(numPackets), 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("--- Error sending total number of packets to the server. ");
        return 1;
    }

    // Buffer to hold each packet
    char buffer[BUFFER_SIZE];
    for (int packetNumber = 0; packetNumber < numPackets; ++packetNumber) {
        // Read a chunk of the file
        imageFile.read(buffer, BUFFER_SIZE);
        // Send the chunk to the server
        if (sendto(clientSocket, buffer, imageFile.gcount(), 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            perror("--- Error sending packet to the server.");
            return 1;
        }
    }

    // To send over the method of image transformation and its arguments from command line
    // Calculate the total length of the arguments
    int totalArgumentsLength = 0;
    for (int i = 0; i < argc; ++i) {
        totalArgumentsLength += std::strlen(argv[i]);
    }
    
    // Allocate memory for the concatenated string arguments
    char* concatenatedArguments = new char[totalArgumentsLength + argc - 1];
    
    // Concatenate the arguments into a string
    strcpy_s(concatenatedArguments, totalArgumentsLength + argc - 1, argv[3]);
    
    /* Starting from the transformation method entered, concatenate the strings together separated by the "|" symbol
           - Example) ./distributed_img_client 127.0.0.1:12345 input_img.png Resizing 300 400
                       will be concatenated into "Resizing|300|400" to be sent over to the server.*/
    for (int i = 4; i < argc; ++i) {
        strcat_s(concatenatedArguments, totalArgumentsLength + argc - 1, "|");
        strcat_s(concatenatedArguments, totalArgumentsLength + argc - 1, argv[i]);
    }
    
    // Sends the concatenated string of transformation method and arguments to the server
    const char* instructionSend = concatenatedArguments;
    if (sendto(clientSocket, instructionSend, strlen(instructionSend), 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("--- Error sending command to the server.");
    }
    else {
        std::cout << "<-- Command message sent to provided IP Address." << std::endl; // Outputs to the user that transformation information was sent to the server
    }
    delete[] concatenatedArguments; // Clear memory

    // Receive the altered image
    const char* invalidSignal = "Invalid"; // Constant variable to check if the received message from the server contains the word "Invalid"
    while (true) {
        // While loop set to true to keep listening for receiving messages from the server
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        // Receive messages from the server
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            std::cout << "--> Server: " << buffer << std::endl; // Displays message received from server

            // Check to see if the received message is the termination signal
            const char* terminationSignal = "Image transmission completed.";
            if (strcmp(buffer, terminationSignal) == 0) {
                // If the client receives "Image transmission completed" from the server, then the client stops listening for messages from the server and breaks the while loop to go straight to closing the socket
                std::cout << "--- Received termination signal. Image transmission completed. " << std::endl;
                break;
            }
            else if (strstr(buffer, invalidSignal) != nullptr) {
                /* If the received message contains the word "Invalid", then the client stops listening for a response from the server, 
                    and wont attempt to listen for a received encoded image or display any image to the client, and proceed with closing the socket*/
                break;
            }
            else {
                // Receive the size of the encoded image first
                size_t imageSize;
                recvfrom(clientSocket, reinterpret_cast<char*>(&imageSize), sizeof(imageSize), 0, (struct sockaddr*)&serverAddr, &serverAddressLen);
                // Receive the encoded image data in chunks
                std::vector<char> receivedData(imageSize);
                size_t receivedBytes = 0;
                const int chunkSize = 1024;
                while (receivedBytes < imageSize) {
                    size_t remainingBytes = std::min<size_t>(chunkSize, imageSize - receivedBytes);
                    int bytesReceived = recvfrom(clientSocket, receivedData.data() + receivedBytes, remainingBytes, 0, (struct sockaddr*)&serverAddr, &serverAddressLen);

                    if (bytesReceived == -1) {
                        perror("--- Error receiving image data.");
                        break;
                    }

                    receivedBytes += bytesReceived;
                }
                // Decode the received data to cv::Mat
                cv::Mat reconstructedImage = cv::imdecode(receivedData, cv::IMREAD_UNCHANGED);
                if (reconstructedImage.empty()) {
                    std::cerr << "--- Failed to decode received image." << std::endl;
#ifdef _WIN32
                    closesocket(clientSocket);
#else
                    close(clientSocket);
#endif
                    return 1;
                }

                // Save the reconstructed image to the same directory as the original image
                std::string originalImagePath(inputImg);
                size_t lastSlash = originalImagePath.find_last_of("/\\");
                std::string outputPath = originalImagePath.substr(0, lastSlash + 1) + "reconstructed_image.png";
                cv::imwrite(outputPath, reconstructedImage);

                // Display both the original and the processed reconstructed images together
                cv::imshow("Original Image", originalImage);
                cv::imshow("Processed Image", reconstructedImage);
                cv::waitKey(0);

            }
        }
        else if (bytesRead == 0) {
            perror("--! Receiving data failed");
            break;
        }
    }

    // Close the socket
#ifdef _WIN32
    closesocket(clientSocket);
    WSACleanup(); // Cleanup Winsock
#else
    close(clientSocket);
#endif
    return 0;
}