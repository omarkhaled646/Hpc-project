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


double* seqcumulativeprob(double* probarr, int size)

{
	double* cumulativeprobarr = new double[size];
	for (int i = 0; i < size; i++)
	{
		if (i == 0)
			cumulativeprobarr[0] = probarr[0];
		else
			cumulativeprobarr[i] = probarr[i] + cumulativeprobarr[i - 1];
	}
	return cumulativeprobarr;
}


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

int* sequentialIntensityCount(int* image, int pixelsNumber)
{
	int* intensityCount = new int[255]();

	for (int pixel = 0; pixel < pixelsNumber; pixel++)
	{
		int temp = image[pixel];
		intensityCount[temp] += 1;
	}

	return intensityCount;

}

int* parallelIntensityCount(int* image, int pixelsNumber)
{
	int* localIntensityCount = new int[pixelsNumber]();
	int* globalIntensityCount = new int[pixelsNumber]();
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
	MPI_Reduce(localIntensityCount, globalIntensityCount, pixelsNumber, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);


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
	int rank, size;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int image[16];
	int imageSize = 16;
	if (rank == 0) {
		image[0] = 3;
		image[1] = 2;
		image[2] = 4;
		image[3] = 5;
		image[4] = 7;
		image[5] = 7;
		image[6] = 8;
		image[7] = 2;
		image[8] = 3;
		image[9] = 1;
		image[10] = 2;
		image[11] = 3;
		image[12] = 5;
		image[13] = 4;
		image[14] = 6;
		image[15] = 7;

	}
	double no_pixelsPerProcessor = imageSize / size;
	int* pixelsPerProcessor = new int[no_pixelsPerProcessor]();
	int* localIntensityCount = new int[imageSize]();
	int* globalIntensityCount = new int[imageSize]();
	double* globalIntensity = new double[imageSize]();
	double* globalPixelProbArr = new double[imageSize]();
	double* PixelProbPerProcessor = new double[no_pixelsPerProcessor]();
	int* newIntensity = new int[imageSize]();
	int* newIntensityPerProcessor = new int[no_pixelsPerProcessor]();


	MPI_Scatter(image, no_pixelsPerProcessor, MPI_INT, pixelsPerProcessor, no_pixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);

	for (int pixel = 0; pixel < no_pixelsPerProcessor; pixel++)
	{
		int temp = pixelsPerProcessor[pixel];
		localIntensityCount[temp] += 1;
	}
	MPI_Reduce(localIntensityCount, globalIntensityCount, imageSize, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	if (rank == 0)
	{
		for (int i = 0; i < imageSize; i++)
		{
			globalIntensity[i] = (double)globalIntensityCount[i];
		}
	}

	MPI_Scatter(globalIntensity, no_pixelsPerProcessor, MPI_DOUBLE, PixelProbPerProcessor, no_pixelsPerProcessor, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	for (int i = 0; i < no_pixelsPerProcessor; i++)
	{
		if (i == 0)
			PixelProbPerProcessor[i] = PixelProbPerProcessor[i] / imageSize;

		else
			PixelProbPerProcessor[i] = (PixelProbPerProcessor[i] / imageSize) + PixelProbPerProcessor[i - 1];


	}
	double end = PixelProbPerProcessor[(int)no_pixelsPerProcessor - 1];
	double sum = 0;
	MPI_Exscan(&end, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	for (int i = 0; i < no_pixelsPerProcessor; i++)
	{

		newIntensityPerProcessor[i] = (int)((PixelProbPerProcessor[i] + sum) * 20);

	}

	MPI_Gather(newIntensityPerProcessor, no_pixelsPerProcessor, MPI_INT, newIntensity, no_pixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);
	if (rank == 0)
	{
		for (int i = 0; i < 16; i++)
		{
			printf("%d = %d\n", i, newIntensity[i]);
		}
	}
	/*
	int* x = parallelIntensityCount(image, 16);
	for (int i = 0; i < 16; i++)
	{
		printf("%d = %d \n",i, x[i]);
	}
	*/
	MPI_Finalize();

	if (rank == 0)
	{
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

}



