#include "Image.h"

Image::Image() = default; // Default Constructor

Image::Image(cv::Mat & imagePassed) {
	/* - Overloader / Custom Constructor to instantiate an instance of type "Image",
	   with an address refernce to the cv::Mat stored image object read in
	   the main program. */
	image_ = imagePassed.clone();
}

Image::~Image() {}; // Destructor

void Image::Resize(int width, int height) {
	/* - The OpenCV "resize" method takes as arguments:
		- 1) The image you want to resize.
		- 2) An object of type cv::Mat to store the resized output image.
		- 3) The output image size.
		- 3) The size scale factor in pixels along both horizontal and vertical axis',
		to resize the original picture to.*/
	cv::resize(image_, image_, cv::Size(width, height), cv::INTER_LINEAR);
}

void Image::Rotate(double angle) {
	/* - "center" calculates the center of the image given its dimensions, to act as a
	pivot for the angle of rotation.*/
	cv::Point2f center(image_.cols / 2.0, image_.rows / 2.0);
	/* - "rotationMatrix" rotates the image around the calculated 'center' point,
		  the amount of degress specified by the angle function parameter, and the final
		  parameter '1.0' represents the scale - so in this function, '1.0' means we want
		  to keep the image the same scale, and just rotate. */
	cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, angle, 1.0);
	// - "warpAffine" rotates the image around the calculated "rotationMatrix"
	cv::warpAffine(image_, image_, rotationMatrix, image_.size());
}

void Image::Crop(int startX, int startY, int width, int height) {
	/*-"roi" specifies the Region of Interest to focus and copy from the original image.
	 - The width and height parameters specify the size of the cropped image and the size
		of the rectangle that will be the Region of Interest.
	 - Having the starting X and Y pixel coordinates as arguements to specify where the size
		rectangle of width and height will start from within the original picture.
	 - Example) startX = 10, startY = 10, width = 20, height = 30:
		will generate a rectangle of size 20x30 pixels, starting from position 10,10 in
		the original image. */
	cv::Rect roi(startX, startY, width, height);
	image_ = image_(roi).clone(); // Clones the Region of Interest to private image variable. 
}

void Image::Flip(int flipCode) {
	/* OpenCV's Flip method has 3 parameters:
	 - 1) The cv::Mat object as an image input.
	 - 2) The cv::Mat object as the desired image output.
	 - 3) An integer value to represent the type of flip operation:
		 0 for horizontal flip, positive integer for vertical flip, negative integer for both*/
	cv::flip(image_, image_, flipCode);
}

void Image::ContrastAdjust(double adjustContrast) {
	/*- The value of each pixel in the image should be multiplied by a positive constant.
	  - If the constant is greater than 1, the contrast will be increased.
	  - If the constant is between 0 and 1, the contrast will be decreased.
	  - Example) (2 = increase contrast by 2) | (4 = increase contrast by 4)
			 (0.5 = decrease contrast by 0.5) | (0.25 = decrease contrast by 0.25)*/
	image_.convertTo(image_, -1, adjustContrast, 0);
}

void Image::BrightnessAdjust(int adjustBrightness) {
	/*- The value of each pixel in the image should be increased / decreased by a constant.
	  - Positive Integer increases brightness:
			- (50 = increase brightness by 50)
			- (100 = increase brightness by 100)
	  - Negative Integer decreases brightness:
			- (-50 = decrease brightness by 50)
			- (-100 = decrease brightness by 100)*/
	image_.convertTo(image_, -1, 1, adjustBrightness);
}

void Image::Blur(int matrixDimension) {
	if (matrixDimension == 3) {
		// - Blur the image wth a 3x3 Gaussian Kernel.
		cv::GaussianBlur(image_, image_, cv::Size(3, 3), 0);
	}
	else if (matrixDimension == 5) {
		// - Blur the image with a 5x5 Gaussian Kernel.
		cv::GaussianBlur(image_, image_, cv::Size(5, 5), 0);
	}
}

void Image::Sharpen() {
	/* - Sharpening an image with a 2D - convolution kernel matrix.
	   - Matrix below is commonly used for Sharpening. */
	cv::Mat sharpeningKernel = (cv::Mat_<double>(3, 3) << 0, -1, 0,
		-1, 5, -1,
		0, -1, 0);
	cv::filter2D(image_, image_, -1, sharpeningKernel, cv::Point(-1, -1), 0, cv::BORDER_DEFAULT);
	// - "Filter2D" applies an arbitrary linear filter to an image.
}

bool Image::checkResizeArguments(std::vector<std::string> arguments) {
	/* - Boolean check method to only allow Image to execute Resize method, if the arguments given in command line are valid for Resizing
	   - Only returns true if 2 integer arguments are input following "Resizing". */
	try {
		int width = std::stoi(arguments[1]);
		int height = std::stoi(arguments[2]);
		return true;
	}catch (const std::invalid_argument& e) {
		std::cerr << "!-- Invalid argument provided for Resize operation." << std::endl;
		return false;
	}
}

bool Image::checkRotateArguments(std::vector<std::string> arguments) {
	/* - Boolean check method to only allow Image to execute Rotate method, if the arguments given in command line are valid for Rotating
	   - Only returns true if 1 double argument is input following "Rotation". */
	try {
		double angle = std::stod(arguments[1]);
		return true;
	}
	catch (const std::invalid_argument& e) {
		std::cerr << "!-- Invalid argument provided for Rotate operation." << std::endl;
		return false;
	}
}

bool Image::checkCropArguments(std::vector<std::string> arguments) {
	/* - Boolean check method to only allow Image to execute Crop method, if the arguments given in command line are valid for Cropping
	   - Only returns true if 4 integer arguments are input following "Cropping".
	   - Then if all 4 inputs are integers, it will only return true if the dimensions given from the command line are valid to crop the image given its existing dimensions.
	   - Meaning it checks to see, from the starting X & Y coordinates given within the image as a starting position (e.g. x:30, y:40 in the image), if the target copped size is to be 100x100 pixels,
			but the original picture size is only 130x130 pixels, the below if statement would return false as these parameters given would mean the image starting at y:40, to be cropped for a further 100 pixels to y:140,
			exceeds the original image dimension of y:130. Therefore, would return false as the arguements provided are larger than the crop. */
	try {
		int startX = std::stoi(arguments[1]);
		int startY = std::stoi(arguments[2]);
		int width = std::stoi(arguments[3]);
		int height = std::stoi(arguments[4]);
		if (getImage().rows >= height + startY && getImage().cols >= width + startX) {
			return true;
		}
		else { return false; }
	}
	catch (const std::invalid_argument& e) {
		std::cerr << "!-- Invalid argument provided for Crop operation." << std::endl;
		return false;
	}
}

bool Image::checkFlipArguments(std::vector<std::string> arguments) {
	/* - Boolean check method to only allow Image to execute Flip method, if the arguments given in command line are valid for Flipping
	   - Only returns true if 1 integer argument is input following "Flipping". */
	try {
		int flipCode = std::stoi(arguments[1]);
		return true;
	}
	catch (const std::invalid_argument& e) {
		std::cerr << "!-- Invalid argument provided for Flip operation." << std::endl;
		return false;
	}
}

bool Image::checkContrastAdjustArguments(std::vector<std::string> arguments) {
	/* - Boolean check method to only allow Image to execute Contrast Adjustment method, if the arguments given in command line are valid for Adjusting the Contrast
	   - Only returns true if 1 double argument is input following ContrastAdjustment. */
	try {
		double adjustContrast = std::stod(arguments[1]);
		return true;
	}
	catch (const std::invalid_argument& e) {
		std::cerr << "!-- Invalid argument provided for Contrast Adjustment operation." << std::endl;
		return false;
	}
}

bool Image::checkBrightnessAdjustArguments(std::vector<std::string> arguments) {
	/* - Boolean check method to only allow Image to execute Brightness Adjustment method, if the arguments given in command line are valid for Adjusting the Brightness
	   - Only returns true if 1 integer argument is input following BrightnessAdjustment. */
	try {
		int adjustBrightness = std::stoi(arguments[1]);
		return true;
	}
	catch (const std::invalid_argument& e) {
		std::cerr << "!-- Invalid argument provided for Brightness Adjustment operation." << std::endl;
		return false;
	}
}

bool Image::checkBlurArguments(std::vector<std::string> arguments) {
	/* - Boolean check method to only allow Image to execute Blur method, if the arguments given in command line are valid for Blurring
	   - Only returns true if 1 integer argument is input following Blurring. 
	   - Then only if the proceeding argument is an integer, does this method check to see if the integer entered is either a "3" or "5" to represent the dimensions of the matrix dimension.
			If the integer input is neither a "3" or "5", then this method returns false. */
	try {
		int matrixDimension = std::stoi(arguments[1]);
		if (matrixDimension == 3 || matrixDimension == 5) {
			return true;
		}
		else {
			return false;
		}
	}
	catch (const std::invalid_argument& e) {
		std::cerr << "!-- Invalid argument provided for Brightness Adjustment operation." << std::endl;
		return false;
	}
}

const cv::Mat Image::getImage() { return image_; } // Getter function to retrieve the image.