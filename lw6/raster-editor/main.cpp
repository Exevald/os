#include "ImageViewer.h"
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
	try
	{
		std::string filename = "large_image.bin";

		if (argc > 1)
		{
			filename = argv[1];
		}

		std::ifstream file(filename);
		if (!file.good())
		{
			std::cout << "Creating new image file: " << filename << std::endl;
			ImageFile imageFile(filename);
			imageFile.CreateImage(8192, 8192);
		}

		ImageViewer viewer(filename);
		viewer.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}