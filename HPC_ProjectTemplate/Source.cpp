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
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

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
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

int* sequentialIntensityCount(int* image, int pixelsCount)
{
	int* intensityCount = new int[256]();

	for (int pixel = 0; pixel < pixelsCount; pixel++)
	{
		int temp = image[pixel];
		intensityCount[temp] += 1;
	}

	return intensityCount;

}
double* sequentialProbability(int* intensityCount, int pixelsCount)
{
	double* cumulativeProbPerIntensity = new double[256]();
	for (int intensity = 0; intensity < 256; intensity++)
	{
		if (intensity == 0)
			cumulativeProbPerIntensity[intensity] = ((double)intensityCount[intensity]) / pixelsCount;

		else
			cumulativeProbPerIntensity[intensity] = (((double)intensityCount[intensity]) / pixelsCount) + cumulativeProbPerIntensity[intensity - 1];
	}
	return cumulativeProbPerIntensity;
}
int* updateIntensity(double* intensityCount)
{
	int* newIntensityArr = new int[256]();
	for (int intensity = 0; intensity < 256; intensity++)
		newIntensityArr[intensity] = (int)(intensityCount[intensity] * 240);
	return newIntensityArr;
}

int* updateImage(int* imageData, int* intensityArr, int pixelsCount)
{
	int* newImage = new int[pixelsCount]();
	for (int pixel = 0; pixel < pixelsCount; pixel++)
	{
		int intensity = imageData[pixel];
		newImage[pixel] = intensityArr[intensity];
	}
	return newImage;
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
	int pixelsCount = BM.Width * BM.Height;




	start_s = clock();
	int rank, size;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int no_pixelsPerProcessor = pixelsCount / size;
	int* pixelsPerProcessor = new int[no_pixelsPerProcessor]();
	int* localIntensityCount = new int[256]();
	int* globalIntensityCount = new int[256]();
	int* newIntensity = new int[256]();
	int* newImageDataParallel = new int[pixelsCount]();
	int* newImageDataSequential = new int[pixelsCount]();

	MPI_Scatter(imageData, no_pixelsPerProcessor, MPI_INT, pixelsPerProcessor, no_pixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);

	for (int pixel = 0; pixel < no_pixelsPerProcessor; pixel++)
	{
		int temp = pixelsPerProcessor[pixel];
		localIntensityCount[temp] += 1;
	}
	MPI_Reduce(localIntensityCount, globalIntensityCount, 256, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	
	if (rank == 0 && pixelsCount % size != 0)
	{
		int remainingPixels = pixelsCount % size;
		int startIndex = no_pixelsPerProcessor * size;
		
		for (int pixel = startIndex; pixel < pixelsCount; pixel++)
		{
			int temp = imageData[pixel];
			globalIntensityCount[temp] += 1;
		}
	}
	
		if (rank == 0)
	{
		double* ParallelCumulativeProbPerIntensity = sequentialProbability(globalIntensityCount, pixelsCount);
		newIntensity = updateIntensity(ParallelCumulativeProbPerIntensity);
	}

	MPI_Bcast(newIntensity, 256, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Scatter(imageData, no_pixelsPerProcessor, MPI_INT, pixelsPerProcessor, no_pixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);

	for (int pixel = 0; pixel < no_pixelsPerProcessor; pixel++)
	{
		int intestiy = pixelsPerProcessor[pixel];
		pixelsPerProcessor[pixel] = newIntensity[intestiy];
	}
	MPI_Gather(pixelsPerProcessor, no_pixelsPerProcessor, MPI_INT, newImageDataParallel, no_pixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);
	
	if (rank == 0 && pixelsCount % size != 0)
	{
		int remainingPixels = pixelsCount % size;
		int startIndex = no_pixelsPerProcessor * size;

		for (int pixel = startIndex; pixel < pixelsCount; pixel++)
		{
			int intestiy = imageData[pixel];
			newImageDataParallel[pixel] = newIntensity[intestiy];
		}
	}

	MPI_Finalize();

	if (rank == 0)
	{
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time for parallel: " << TotalTime << endl;
		createImage(newImageDataParallel, ImageWidth, ImageHeight, 0);

		start_s = clock();
		int* pixelsPerIntensity = sequentialIntensityCount(imageData, pixelsCount);
		double* sequentialCumulativeProbPerIntensity = sequentialProbability(pixelsPerIntensity, pixelsCount);
		int* newIntenstiySequential = updateIntensity(sequentialCumulativeProbPerIntensity);
		int* newImageSequential = updateImage(imageData, newIntenstiySequential, pixelsCount);
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time for sequential: " << TotalTime << endl;
		createImage(newImageSequential, ImageWidth, ImageHeight, 1);

		free(imageData);
		system("pause");
		return 0;
	}

}



