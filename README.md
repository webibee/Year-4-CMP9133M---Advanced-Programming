# Distributed Image Processing System
- Course: CMP9133M – Advanced Programming
- Institution: University of Lincoln
- Grade Received: 85%
- Language: C++17
- Libraries used: OpenCV, Standard Template Library (STL), Winsock2 (Windows Sockets)

# Overview
This project implements a distributed image processing system using a client-server architecture. Multiple clients can send images to a single server, which applies various image filters concurrently using multi-threading, then returns the processed images to the clients.

The system demonstrates:
- UDP socket programming for network communication
- Multi-threading for concurrent image processing
- OpenCV integration for image manipulation
- Robust error handling for invalid inputs and network failures
- Object-oriented design with UML modelling

# Architecture
## Client-Server Model

## Communication Sequence (UDP)

```mermaid
sequenceDiagram
    participant Client
    participant Network as UDP Network
    participant Server
    participant Image as Image Class
    participant OpenCV

    Client->>Network: Connect (IP:Port)
    Network->>Server: Forward connection
    
    Client->>Network: Send Image Size (4 bytes)
    Network->>Server: Forward size
    
    Client->>Network: Send Image Data (chunks)
    Network->>Server: Forward data
    Server->>Image: Create Image object
    
    Client->>Network: Send Filter Instruction
    Note over Client,Network: Format: "grayscale|" or "blur|5"
    Network->>Server: Forward instruction
    
    alt Valid Filter
        Server->>Image: Apply filter
        Image->>OpenCV: Process image
        OpenCV-->>Image: Return processed image
        Image-->>Server: Filter complete
        Server->>Network: Send Processed Image Size
        Network->>Client: Forward size
        Server->>Network: Send Processed Image Data
        Network->>Client: Forward data
        Client->>Client: Save to disk
        Client->>Client: Display both images
    else Invalid Filter / Arguments
        Server->>Network: Send "Invalid" error
        Network->>Client: Forward error
        Client->>Client: Display error message
    end
    
    Client->>Network: Close connection
    Network->>Server: Connection closed
 ```

## UML Class Diagram (Image Class)
<img width="390" height="289" alt="image" src="https://github.com/user-attachments/assets/5039c055-6300-4227-b6d8-8c0f90605335" />


## UML Sequence Diagram

<img width="484" height="454" alt="image" src="https://github.com/user-attachments/assets/9e475063-bede-4693-a730-62044617752d" />


# Key Features 
- Console application (server + client)
- Client-server architecture (UDP sockets)
- Multi-threaded image processing on server
- Multiple image filters (implemented via OpenCV)
- Client-side image display and saving

# Advanced Features
- Thread-safe processing – Multiple client requests handled concurrently
- UDP with reliability – Image size header + chunked reassembly
- Custom protocol – [transformation|arg1|arg2|...] format
- Robust error handling – Invalid IPs, ports, file paths, filter arguments
- OpenCV integration – Industry-standard computer vision library

# Image Filters Supported
Category - Filters
- Geometric - Resize, Rotate, Crop, Flip (horizontal/vertical)
- Colour - Grayscale, Brightness, Contrast, Gamma Correction, RGB↔HSV
- Blur/Sharpen - Gaussian Blur, Box Blur, Sharpening

# Client Launch Format
/distributed_img_client <server_ip:port> <image_path> <transformation> [parameters...]
