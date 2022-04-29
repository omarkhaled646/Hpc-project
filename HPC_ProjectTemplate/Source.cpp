#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#include <mpi.h>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int *Red = new int[BM.Height * BM.Width];
	int *Green = new int[BM.Height * BM.Width];
	int *Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height*BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i*BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	
	return input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i*width + j] < 0)
			{
				image[i*width + j] = 0;
			}
			if (image[i*width + j] > 255)
			{
				image[i*width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

int* sequentialIntensityCount(int* image, int pixelsNumber)
{
	int* intensityCount = new int[255]();
	
	for (int pixel = 0; pixel < pixelsNumber ; pixel++)
	{
		int temp = image[pixel];
		intensityCount[temp] += 1;
	}
	
	return intensityCount;

}

int* parallelIntensityCount(int* image, int pixelsNumber)
{
	int* localIntensityCount = new int[255]();
	int* globalIntensityCount = new int[255]();
	int rank, size;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int no_pixelsPerProcessor = pixelsNumber / size;
	int* pixelsPerProcessor = new int[no_pixelsPerProcessor]();
	MPI_Scatter(image, no_pixelsPerProcessor, MPI_INT, pixelsPerProcessor, no_pixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);

	for (int pixel = 0; pixel < no_pixelsPerProcessor; pixel++)
	{
		int temp = pixelsPerProcessor[pixel];
		localIntensityCount[temp] += 1;
	}
	MPI_Reduce(localIntensityCount, globalIntensityCount, 255, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  
	if (rank == 0)
	{
		return globalIntensityCount;
	}
	MPI_Finalize();
	
}

int main()
{
	int ImageWidth = 4, ImageHeight = 4;

	int start_s, stop_s, TotalTime = 0;

	System::String^ imagePath;
	std::string img;
	img = "..//Data//Input//test.png";
	
	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	System::Drawing::Bitmap BM(imagePath);
    int pixelsNumber = BM.Width * BM.Height;

   
	start_s = clock();
	int* parallelCount = parallelIntensityCount(imageData, pixelsNumber);	
	stop_s = clock();
	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	cout << "time for parallel: " << TotalTime << endl;
	
	start_s = clock();
	int* sequentialCount = sequentialIntensityCount(imageData, pixelsNumber);
	stop_s = clock();
	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	cout << "time for sequential: " << TotalTime << endl;
	//createImage(imageData, ImageWidth, ImageHeight, 0);

	//free(imageData);
	system("pause");
	return 0;
	
}



