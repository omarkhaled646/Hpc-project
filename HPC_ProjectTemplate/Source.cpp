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
	for (int pixel = 0; pixel < pixelsCount; pixel++)
	{
		if (pixel == 0)
			cumulativeProbPerIntensity[pixel] =((double)intensityCount[pixel]) / pixelsCount;

		else
			cumulativeProbPerIntensity[pixel] = (((double)intensityCount[pixel]) / pixelsCount) + cumulativeProbPerIntensity[pixel - 1];
	}
	return cumulativeProbPerIntensity;
}
int* newIntensityArr(double* intensityCount, int pixelsCount)
{
	int* newIntensityArr = new int[256]();
	for(int intensity =0; intensity < pixelsCount; intensity++)
		newIntensityArr[intensity]=(int)(intensityCount[intensity] * 240);
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
	int no_intenstiesPerProcessor = 256 / size;
	int* pixelsPerProcessor = new int[no_pixelsPerProcessor]();
	int* localIntensityCount = new int[pixelsCount]();
	int* globalIntensityCount = new int[pixelsCount]();
	double* globalIntensity = new double[pixelsCount]();
	double* IntensityProbPerProcessor = new double[no_intenstiesPerProcessor]();
	int* newIntensity = new int[pixelsCount]();
	int* newIntensityPerProcessor = new int[no_pixelsPerProcessor]();
	int* newImageDataParellel = new int[pixelsCount]();
	int* newImageDataSequential = new int[pixelsCount]();

	MPI_Scatter(imageData, no_pixelsPerProcessor, MPI_INT, pixelsPerProcessor, no_pixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);

	for (int pixel = 0; pixel < no_pixelsPerProcessor; pixel++)
	{
		int temp = pixelsPerProcessor[pixel];
		localIntensityCount[temp] += 1;
	}
	MPI_Reduce(localIntensityCount, globalIntensityCount, pixelsCount, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	if (rank == 0)
	{
		for (int pixel = 0; pixel < 256; pixel++)
		{
			globalIntensity[pixel] = (double)globalIntensityCount[pixel];
		}
	}

	MPI_Scatter(globalIntensity, no_intenstiesPerProcessor, MPI_DOUBLE, IntensityProbPerProcessor, no_intenstiesPerProcessor, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	for (int pixel = 0; pixel < no_intenstiesPerProcessor; pixel++)
	{
		if (pixel == 0)
			IntensityProbPerProcessor[pixel] = IntensityProbPerProcessor[pixel] / pixelsCount;

		else
			IntensityProbPerProcessor[pixel] = (IntensityProbPerProcessor[pixel] / pixelsCount) + IntensityProbPerProcessor[pixel - 1];


	}
	double lastComulativeProb = IntensityProbPerProcessor[(int)no_intenstiesPerProcessor - 1];
	double sum = 0;
	MPI_Exscan(&lastComulativeProb, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	for (int intensity = 0; intensity < no_intenstiesPerProcessor; intensity++)
	{

		newIntensityPerProcessor[intensity] = (int)((IntensityProbPerProcessor[intensity] + sum) * 240);

	}

	MPI_Gather(newIntensityPerProcessor, no_intenstiesPerProcessor, MPI_INT, newIntensity, no_intenstiesPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(newIntensity, 256, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Scatter(imageData, no_pixelsPerProcessor, MPI_INT, pixelsPerProcessor, no_pixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);
	
	for (int pixel = 0; pixel < no_pixelsPerProcessor; pixel++)
	{
		int intestiy = pixelsPerProcessor[pixel];
		pixelsPerProcessor[pixel] = newIntensity[intestiy];
	}
	MPI_Gather(pixelsPerProcessor, no_pixelsPerProcessor, MPI_INT, newImageDataParellel, no_pixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Finalize();

	if (rank == 0)
	{
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time for parallel: " << TotalTime << endl;
		createImage(newImageDataParellel, ImageWidth, ImageHeight, 0);

		/*start_s = clock();
		int* pixelsPerIntensity = sequentialIntensityCount(imageData, pixelsCount);
		double* cumulativeProbPerIntensity = sequentialProbability(pixelsPerIntensity, pixelsCount);
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time for sequential: " << TotalTime << endl;*/

		free(imageData);
		system("pause");
		return 0;
	}

}



