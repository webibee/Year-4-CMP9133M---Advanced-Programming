#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <numeric>
#include <algorithm>
#include <opencv2/opencv.hpp> // - Include the OpenCV header file to use its methods

#include "Image.h" // - Include the header file of the Image class

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // - Link the Winsock library
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#endif

const int BUFFER_SIZE = 1024; // - Program constant of setting the buffer size to be the same throughout the program

// - Enumeration to define cases for switch statement in the main program for image transformations
enum Transformation {
    Resizing,
    Rotation,
    Cropping,
    Flipping,
    ContrastAdjustment,
    BrightnessAdjustment,
    Blurring,
    Sharpening,
    Invalid
};

/* - Method to check if the string user input is one of the valid instance cases in the enumeration "Transformation".
   - Will only return one of the enumeration transformations, as they are the only options in the switch case in the main program. */
Transformation getTransformation(const std::string& input) {
    if (input == "Resizing") return Resizing;
    if (input == "Rotation") return Rotation;
    if (input == "Cropping") return Cropping;
    if (input == "Flipping") return Flipping;
    if (input == "ContrastAdjustment") return ContrastAdjustment;
    if (input == "BrightnessAdjustment") return BrightnessAdjustment;
    if (input == "Blurring") return Blurring;
    if (input == "Sharpening") return Sharpening;
    return Invalid;
}

int main() {
#ifdef _WIN32
    // - Initialize Winsock on Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Initialization failed"
            << std::endl;
        return 1;
    }
#endif

    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t serverAddrLen = sizeof(serverAddr);
    socklen_t clientAddrLen = sizeof(clientAddr);

    // - Create a UDP socket
    if ((serverSocket = socket(AF_INET,
        SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        perror("Socket creation failed"); // - SOCK_DATAGRAM utilises UDP
#ifdef _WIN32
        WSACleanup(); // - Cleanup Winsock
#endif
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345); // - Port number
    serverAddr.sin_addr.s_addr = INADDR_ANY; // - Accept connections on any network interface

    // - Bind the socket
    if (bind(serverSocket, (const struct sockaddr*)
        &serverAddr, sizeof(serverAddr)) == -1) {
        perror("Binding failed");
#ifdef _WIN32
        closesocket(serverSocket);
        WSACleanup(); // - Cleanup Winsock
#else
        close(serverSocket);
#endif
        return 1;
    }

    std::cout << "--- Server is listening on port 12345..." << std::endl;

    // - Send / Receive
    // - Handle communication with the client
    char buffer[BUFFER_SIZE]; // Creates the buffer under the globally defined buffer size at the start of the program. 
    
    /* - "transformationPerformed":: Defining boolean globally, for later checking before trying to send an image back to the client.
       - This bool will remain false if any transformation is not carried out or to represent an "Invalid" message was generated throughout the program.
       - If remained false, the image(s) would not be sent back and displayed to the client, as the image would not have been transformed.
    */
    bool transformationPerformed = false; 

    while (true) {
        // - Declaration of variables needed for receiving and sending messages within UDP.
        const char* response;
        size_t responseLength;
        socklen_t len;
        len = sizeof(clientAddr);
        int numPackets;
        int n;;
        
        // - Receive the total number of packets
        // - Else, state error to both server and client stating there was an error receiving the total number of packets
        if (recvfrom(serverSocket, reinterpret_cast<char*>(&numPackets), sizeof(numPackets), 0, (struct sockaddr*)&clientAddr, &clientAddrLen) == -1) {
            perror("Error receiving total number of packets");
            response = "Sever: Error receiving total number of packets";
            responseLength = strlen(response);
            sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
            std::cout << "Error message sent to client." << std::endl;
            break;
        }

        std::ofstream outputFile("received_image.jpg", std::ios::binary); // - A file to write the received data packets to
        for (int packetNumber = 0; packetNumber < numPackets; ++packetNumber) {
            char buffer[BUFFER_SIZE];
            // - Receive a packet
            int bytesReceived = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
            if (bytesReceived == -1) {
                // - Else, state error to both server and client stating there was an error receiving a packet
                perror("Error receiving packet");
                response = "Sever: Error receiving packet";
                responseLength = strlen(response);
                sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                std::cout << "Error message sent to client." << std::endl;
                break;
            }
            // - Write the received chunk to the output file
            outputFile.write(buffer, bytesReceived);
        }
        outputFile.close(); // - Close file once all bytes have been received and written to file called 'received_image.jpg'
        cv::Mat image; // - Create instance of cv::Mat image to store the saved image received from the client
        Image img; // - Create an instance of Image class, ready to try and perform transformations on it 

        // - try block to attempt to read the received image from the client into the 'img' instance of the 'Image' class
        try {
            image = cv::imread("received_image.jpg");
            if (image.empty()) {
                throw cv::Exception(0, "Image not fully received or invalid", "main", "cv::imread", 42);
            }
            img = Image(image);
        } 
        catch (const cv::Exception& e){
            // - Else, state error to both server and client stating server was unable to instantiate the 'Image' class from the client's image that was sent. 
            response = "Unable to instantiate an image class from received image.";
            perror(e.msg.c_str());
            responseLength = strlen(response);
            sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
            std::cout << "--> Image reading to class instantiation error message sent to Client." << std::endl;
            break;
        }

        // - After instantiating object of class Image, attempt to receive transformation instruction and parameters provided from the client's side command line execution.
        len = sizeof(clientAddr);
        n = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &len);
        if (n == -1) {
            perror("Error receiving instruction from client.");
            break;
        }
        buffer[n] = '\0'; // - Null-terminate the received data
        std::string transformation;
        if (n >= 0 && n < BUFFER_SIZE) {
            // - Converts message received from user to inputs in a vector
            buffer[n] = '\0';
            printf("\n<-- Client : %s\n", buffer); // - Outputs received instruction message from client to Server window
            std::string inputString(buffer);
            std::istringstream iss(inputString);
            std::vector<std::string> clientInput;
            std::string token;
            /* - The transformation instruction and parameters given are sent in the format "<transformationMethod>|<arg1>|<arg2>|<arg3>" - separated by the "|" character.
                    E.g.) Resizing|300|400
               - The while loop below iterates through this received string buffer, separating each element by the "|" character, and storing the element into the next position of the string vector 'clientInput'.
                    E.g.) Input "Resizing|300|400" gets stored in 'clientInput' as ["Resizing", "300", "400"].
            */
            while (std::getline(iss, token, '|')) {
                clientInput.push_back(token);
            }
   
            transformation = clientInput[0]; // Retrieves the string transformation that was sent.
            /* - Uses the getTransformation to return the enumeration type corresponding to where the user input's transformation returns true.
                    E.g.) Input "Resizing" in the getTransformation() method will return:: "if (input == "Resizing") return Resizing;".
                          Input "Fish" in the getTransformation() method will return:: "return Invalid;" - as the else statement of the function.
            */
            Transformation transform = getTransformation(transformation); 
            std::thread Thread; // Creates a thread

            /* - Switch case to check the output of the getTransformation() function and enumeration "Transformation"
               - The valid transformation entered by the user will execute the nested portion of code related to running the corresponding Image class method.
                    E.g.) User Input "Resizing" gets sent to the getTransformation() function; returns the Transformation enumeration type 'return Resizing'; which then executes the nested section of code under 'case Resizing'.
                    E.g.) User Input "Fish" gets sent to the getTransformation() function; returns the Transformation enumeration type 'return Invalid'; which then executes the nested section of code under 'case Invalid:'.
               - Each of the cases that require a valid user input arguments check [Resizing, Rotating, Cropping, Flipping, Contrast Adjustment, Brightness Adjustment, Blurring], all have an immediate if else statement 
                    to run the Image Class' check arguement functions.
                    E.g.) case Resizing executes, then the Image Class checkResizeArguments() method executes with the client Input string "Resizing|300|400", and only if the two inputs X and Y in "Resizing|X|Y" are integers, 
                          will the if/true part of the statement execute, otherwise the else/false section will.
               - The use of std::thread allows the server to perform image transformations concurrently as each transformation is executed in a separate thread,
                    allowing the server to handle multiple transformation requests simultanesouly if multi-client support was implemented.     
            */
            switch (transform) {
            case Resizing:
                if (img.checkResizeArguments(clientInput) == true) {
                    /* - Outputs Resizing being carried out onto server; Sends message to client to say Resizing is being executed
                       - Executes the Resize method of the Image class on a thread, passing the received arguments as function parameters:
                            - In Resizing's case: converting 2 strings to integers. 
                       - Sets "transformationPerformed" to true to indicate a transformation on the image has been processed. */
                    std::cout << "--- Performing Resizing on Image." << std::endl;
                    Thread = std::thread(&Image::Resize, std::ref(img), std::stoi(clientInput[1]), std::stoi(clientInput[2]));
                    response = "Processing Resizing Output in threads";
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    transformationPerformed = true;
                }
                else {
                    /* - This section of code is executed if the user entered "Resizing" as the transformation, but passed invalid arguments for it to execute
                       - Outputs Invalid arguments entered for Resizing out onto server; Sends similar error message to client to say Resizing arguments were invalid */
                    response = "Invalid arguments sent for Resizing transformation.";
                    std::cout << "--- " << response << std::endl;
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    std::cout << "--> Resizing arguments error message sent to Client." << std::endl;
                }
                break;
            case Rotation:
                if (img.checkRotateArguments(clientInput) == true) {
                    /* - Outputs Rotation being carried out onto server; Sends message to client to say Rotation is being executed
                       - Executes the Rotation method of the Image class on a thread, passing the received arguments as function parameters:
                           - In Rotation's case: converting 1 string to double.
                       - Sets "transformationPerformed" to true to indicate a transformation on the image has been processed. */
                    std::cout << "--- Performing Rotation on Image." << std::endl;
                    Thread = std::thread(&Image::Rotate, std::ref(img), std::stod(clientInput[1]));
                    response = "Processing Rotation Output in threads";
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    transformationPerformed = true;
                }
                else {
                    /* - This section of code is executed if the user entered "Rotation" as the transformation, but passed invalid arguments for it to execute
                       - Outputs Invalid arguments entered for Rotation out onto server; Sends similar error message to client to say Rotation arguments were invalid */
                    response = "Invalid arguments sent for Rotation transformation.";
                    std::cout << "--- " << response << std::endl;
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    std::cout << "--> Rotation arguments error message sent to Client." << std::endl;
                }
                break;
            case Cropping:
                if (img.checkCropArguments(clientInput) == true) {
                    /* - Outputs Cropping being carried out onto server; Sends message to client to say Cropping is being executed
                       - Executes the Cropping method of the Image class on a thread, passing the received arguments as function parameters:
                           - In Cropping's case: converting 4 strings to integers. 
                       - Sets "transformationPerformed" to true to indicate a transformation on the image has been processed. */
                    std::cout << "--- Performing Cropping on Image." << std::endl;
                    Thread = std::thread(&Image::Crop, std::ref(img), std::stoi(clientInput[1]), std::stoi(clientInput[2]), std::stoi(clientInput[3]), std::stoi(clientInput[4]));
                    response = "Processing Cropping Output in threads";
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    transformationPerformed = true;
                }
                else {
                    /* - This section of code is executed if the user entered "Cropping" as the transformation, but passed invalid arguments for it to execute
                       - Outputs Invalid arguments entered for Cropping out onto server; Sends similar error message to client to say Cropping arguments were invalid */
                    response = "Invalid arguments sent for Cropping transformation.";
                    std::cout << "--- " << response << std::endl;
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    std::cout << "--> Cropping arguments error message sent to Client." << std::endl;
                }
                break;
            case Flipping:
                if (img.checkFlipArguments(clientInput) == true) {
                    /* - Outputs Flipping being carried out onto server; Sends message to client to say Flipping is being executed
                       - Executes the Flipping method of the Image class on a thread, passing the received arguments as function parameters:
                           - In Flipping's case: converting 1 string to integer. 
                       - Sets "transformationPerformed" to true to indicate a transformation on the image has been processed. */
                    std::cout << "--- Performing Flipping on Image." << std::endl;
                    Thread = std::thread(&Image::Flip, std::ref(img), std::stoi(clientInput[1]));
                    response = "Processing Flipping Output in threads";
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    transformationPerformed = true;
                }
                else {
                    /* - This section of code is executed if the user entered "Flipping" as the transformation, but passed invalid arguments for it to execute
                       - Outputs Invalid arguments entered for Flipping out onto server; Sends similar error message to client to say Flipping arguments were invalid */
                    response = "Invalid arguments sent for Flipping transformation.";
                    std::cout << "--- " << response << std::endl;
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    std::cout << "--> Flipping arguments error message sent to Client." << std::endl;
                }
                break;
            case ContrastAdjustment:
                if (img.checkContrastAdjustArguments(clientInput) == true) {
                    /* - Outputs Contrast Adjustment being carried out onto server; Sends message to client to say Contrast Adjustment is being executed
                       - Executes the Contrast Adjustment method of the Image class on a thread, passing the received arguments as function parameters:
                           - In Contrast Adjustment's case: converting 1 string to double. 
                       - Sets "transformationPerformed" to true to indicate a transformation on the image has been processed. */
                    std::cout << "--- Performing Contrast Adjustment on Image." << std::endl;
                    Thread = std::thread(&Image::ContrastAdjust, std::ref(img), std::stod(clientInput[1]));
                    response = "Processing Contrast Adjustment Output in threads";
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    transformationPerformed = true;
                }
                else {
                    /* - This section of code is executed if the user entered "ContastAdjustment" as the transformation, but passed invalid arguments for it to execute
                       - Outputs Invalid arguments entered for Contast Adjustment out onto server; Sends similar error message to client to say Contrast Adjustment arguments were invalid */
                    response = "Invalid arguments sent for Contrast Adjustment transformation.";
                    std::cout << "--- " << response << std::endl;
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    std::cout << "--> Contrast Adjustment arguments error message sent to Client." << std::endl;
                }
                break;
            case BrightnessAdjustment:
                if (img.checkBrightnessAdjustArguments(clientInput) == true) {
                    /* - Outputs Brightness Adjustment being carried out onto server; Sends message to client to say Brightness Adjustment is being executed
                       - Executes the Brightness Adjustment method of the Image class on a thread, passing the received arguments as function parameters:
                           - In Brightness Adjustment's case: converting 1 string to integer. 
                       - Sets "transformationPerformed" to true to indicate a transformation on the image has been processed. */
                    std::cout << "--- Performing Brightness Adjustment on Image." << std::endl;
                    Thread = std::thread(&Image::BrightnessAdjust, std::ref(img), std::stoi(clientInput[1]));
                    response = "Processing Brightness Adjustment Output in threads Output";
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    transformationPerformed = true;
                }
                else {
                    /* - This section of code is executed if the user entered "BrightnessAdjustment" as the transformation, but passed invalid arguments for it to execute
                       - Outputs Invalid arguments entered for Brightness Adjustment out onto server; Sends similar error message to client to say Brightness Adjustment arguments were invalid */
                    response = "Invalid arguments sent for Brightness Adjustment transformation.";
                    std::cout << "--- " << response << std::endl;
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    std::cout << "--> Brightness Adjustment arguments error message sent to Client." << std::endl;
                }
                break;
            case Blurring:
                if (img.checkBlurArguments(clientInput) == true) {
                    /* - Outputs Blurring being carried out onto server; Sends message to client to say Blurring is being executed
                       - Executes the Blurring method of the Image class on a thread, passing the received arguments as function parameters:
                           - In Blurring's case: converting 1 string to integer. 
                       - Sets "transformationPerformed" to true to indicate a transformation on the image has been processed. */
                    std::cout << "--- Performing Blurring on Image." << std::endl;
                    Thread = std::thread(&Image::Blur, std::ref(img), std::stoi(clientInput[1]));
                    response = "Processing Blurring Output in threads";
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    transformationPerformed = true;
                }
                else {
                    /* - This section of code is executed if the user entered "Blurring" as the transformation, but passed invalid arguments for it to execute
                       - Outputs Invalid arguments entered for Brightness out onto server; Sends similar error message to client to say Blurring arguments were invalid */
                    response = "Invalid arguments sent for Blurring transformation.";
                    std::cout << "--- " << response << std::endl;
                    responseLength = strlen(response);
                    sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                    std::cout << "--> Blurring arguments error message sent to Client." << std::endl;
                }
                break;
            case Sharpening:
                /* - Outputs Sharpening being carried out onto server; Sends message to client to say Sharpening is being executed
                   - Executes the Blurring method of the Image class on a thread. 
                   - Sets "transformationPerformed" to true to indicate a transformation on the image has been processed. */
                std::cout << "--- Performing Sharpening on Image." << std::endl;
                Thread = std::thread(&Image::Sharpen, std::ref(img));
                response = "Processing Sharpening Output in threads";
                responseLength = strlen(response);
                sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                transformationPerformed = true;
                break;
            case Invalid:
                // - Outputs Invalid Transformation Error Message onto server; Sends message to client to say the same error message. 
                std::cout << "--- Invalid Transformation Entered." << std::endl;
                response = "Invalid Transformation Method entered.";
                responseLength = strlen(response);
                sendto(serverSocket, response, responseLength, 0, (const struct sockaddr*)&clientAddr, len);
                std::cout << "--> Transformation Method Input Error Message sent to Client." << std::endl;
                break;
            }

            // Join threads again outside of the switch statement
            if (Thread.joinable()) {
                Thread.join();
            }

            /* - Will only execute the following block of code if the transformationPerformed boolean is set to true
               - If set to true, this means a transformation was carried out on the image, otherwise the bool remained false from the declaration at the start of the program
               - If true, the processed image is encoded, then the size of the encoded data is sent to the user, followed by the encoded image data itself; 
                    then a termination signal is sent to the user to signify the image sent has been transformed and sent back to the client, 
                    which triggers the client side to stop listening for messages from the server.*/
            if (transformationPerformed == true) {
                // Send Back
                cv::Mat processedImage = img.getImage();
                // Encodes processed image
                std::vector<uchar> encodedImage;
                cv::imencode(".png", processedImage, encodedImage);
                // Calculates size of encoded image, to send to client so they know how much data to receive
                size_t imageSize = encodedImage.size();
                // Send the size of the encoded image first
                sendto(serverSocket, reinterpret_cast<char*>(&imageSize), sizeof(imageSize), 0, (const struct sockaddr*)&clientAddr, len);
                // After size of encoded image is sent, Send the encoded image data
                const int chunkSize = 1024;
                for (size_t offset = 0; offset < imageSize; offset += chunkSize) {
                    size_t remainingBytes = std::min<size_t>(chunkSize, imageSize - offset);
                    sendto(serverSocket, reinterpret_cast<char*>(encodedImage.data() + offset), remainingBytes, 0, (const struct sockaddr*)&clientAddr, len);
                }

                /* - Send the termination signal
                   - Termination signal signifies processed image is sent back to client, and is ready for both server and client to close sockets. */
                const char* terminationSignal = "Image transmission completed.";
                size_t terminationSignalLength = strlen(terminationSignal);
                sendto(serverSocket, terminationSignal, terminationSignalLength, 0, (const struct sockaddr*)&clientAddr, len);
                std::cout << "--> Processed Output Image and Termination Signal sent to Client." << std::endl;
                break;
            }
            
        }
        else {
            printf("Error receiving data.\n");
            break;
        }
        
        // Destructor of the Image class called as it has gone out of scope. 
    }

    // Close the sockets
#ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup(); // Cleanup Winsock
#else
    close(clientSocket);
    close(serverSocket);
#endif

    return 0;
};