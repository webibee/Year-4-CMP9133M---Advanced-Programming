#pragma once

#include <opencv2/opencv.hpp>
class Image
{
	// A class representing an image with various image processing operations. 
public:
	// Default constructor for the Image class.
	Image(); 
	// Override Constructor to intialise instance of Image with a provided OpenCV Mat.
	Image(cv::Mat& imagePassed); 
	// Class destructor.
	~Image(); 
	/*
	 - function: Resize the image to the specified width and height.
	 - parameter: width: The new width of the image.
	 - parameter: height: The new height of the image. 
	*/
	void Resize(int width, int height);
	/*
	 - function: Rotates the image by the specified angle.
	 - parameter: angle: The angle in degrees in which the image will be rotated.
	*/
	void Rotate(double angle);
	/*
	 - function: Crops the image by the specified region.
	 - parameter: startX: The x-coordinate of the starting position of the cropped image.
	 - parameter: startY: The y-coordinate of the starting position of the cropped image.
	 - parameter: width: The width of the cropped image.
	 - parameter: height: The height of the cropped image.
	*/
	void Crop(int startX, int startY, int width, int height);
	/*
	 - function: Flips the image horizontally, vertically or both.
	 - parameter: flipCode: The integer representing the type of flip to be carried out.
	*/
	void Flip(int flipCode);
	/*
	 - function: Changes the contrast of the image.
	 - parameter: adjustContrast: The double representing the amount to change the contrast by.
	*/
	void ContrastAdjust(double adjustContrast);
	/*
	 - function: Changes the brightness of the image.
	 - parameter: adjustBrightness: The integer representing the amount to change the brightness by.
	*/
	void BrightnessAdjust(int adjustBrightness);
	/*
	 - function: Blurs the image.
	 - parameter: matrixDimension: The integer representing the dimensions of the matrix used to blur the image.
	*/
	void Blur(int matrixDimension);
	
	// - function: Sharpens the image.
	void Sharpen();
	/*
	 - function: Checks the command line arguments provided to see if the Resize function can run with the correct arguments without raising an error.
	 - parameter: arguments: The strings entered in the command line by the user.
	*/
	bool checkResizeArguments(std::vector<std::string> arguments);
	/*
	 - function: Checks the command line arguments provided to see if the Rotate function can run with the correct arguments without raising an error.
	 - parameter: arguments: The strings entered in the command line by the user.
	*/
	bool checkRotateArguments(std::vector<std::string> arguments);
	/*
	 - function: Checks the command line arguments provided to see if the Crop function can run with the correct arguments without raising an error.
	 - parameter: arguments: The strings entered in the command line by the user.
	*/
	bool checkCropArguments(std::vector<std::string> arguments);
	/*
	 - function: Checks the command line arguments provided to see if the Flip function can run with the correct arguments without raising an error.
	 - parameter: arguments: The strings entered in the command line by the user.
	*/
	bool checkFlipArguments(std::vector<std::string> arguments);
	/*
	 - function: Checks the command line arguments provided to see if the Contrast Adjust function can run with the correct arguments without raising an error.
	 - parameter: arguments: The strings entered in the command line by the user.
	*/
	bool checkContrastAdjustArguments(std::vector<std::string> arguments);
	/*
	 - function: Checks the command line arguments provided to see if the Brightness Adjust function can run with the correct arguments without raising an error.
	 - parameter: arguments: The strings entered in the command line by the user.
	*/
	bool checkBrightnessAdjustArguments(std::vector<std::string> arguments);
	/*
	 - function: Checks the command line arguments provided to see if the Blur function can run with the correct arguments without raising an error.
	 - parameter: arguments: The strings entered in the command line by the user.
	*/
	bool checkBlurArguments(std::vector<std::string> arguments);
	/*
	 - function: Getter function to retrieve the private member cv::Mat.
	*/
	const cv::Mat getImage();

private:
	// The OpenCV Mat representing the image data. 
	cv::Mat image_;
};
