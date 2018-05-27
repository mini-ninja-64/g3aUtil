#include <iostream>
#include <string>
#include <SDL2/SDL.h>

//TODO: optimise
//TODO write own bitmap reader and loader

/*
how to bitmap - simple, cba to write fancy reader lolll ( that why we missing the pallete ), windows bitmap, damn this is a simple bitmap fuk me lol

|---------------|
|    header		|--type of bitmap(2 byte), size of file(4), reserved1 (2), reserved2 (2), offset to image data(4)
|---------------|
|				|
|  info header	|--size of header (4), w (4), h (4), num of colour planes(2), bits per pixel(2), compression(4), ppm h (4), ppm v (4), num of colours (4), num of important colours (4)
|				|
|---------------|
|				|
|   IMAGE DATA	|--24bit bgr
|				|
|---------------|
*/

struct pixel
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

int writeBMP(char const* filename, unsigned int width, unsigned int height, pixel imagePixels[92][64]) {
	SDL_Surface* surf;
	surf = SDL_CreateRGBSurfaceWithFormat(0, width, height, 24, SDL_PIXELFORMAT_RGB888);
	if (surf == NULL) {
		SDL_Log("SDL_CreateRGBSurfaceWithFormat() failed: %s", SDL_GetError());
		exit(1);
	}
	unsigned char* surfPixels = (unsigned char*)surf -> pixels;
	for (int x = 0; x < width; x++){
		for (int y = 0; y < height; y++){
			surfPixels[4 * (y * width + x)] = imagePixels[x][y].b;
			surfPixels[4 * (y * width + x)+1] = imagePixels[x][y].g;
			surfPixels[4 * (y * width + x)+2] = imagePixels[x][y].r;
		}
	}
	//sstd::cout << SDL_GetPixelFormatName(surf->format->format) << std::endl;
	SDL_SaveBMP(surf, filename);
	SDL_FreeSurface(surf);
	return 0;
}

int extractImage(char const* imageOut, char const* g3a, int address){
	unsigned char buffer[0x2E00];
	pixel pixelArray[92][64];

	FILE *fptr;
	fptr = fopen(g3a,"rb");
	fseek ( fptr , address , SEEK_SET );
	fread(buffer, sizeof(unsigned char), 0x2E00, fptr);
	fclose(fptr);

	int iX = 0;
	int iY = 0;
	for (int i = 0; i < 0x2E00; i+=2 ){
		//pixelArray[iX][iY].r = 0xFF*((buffer[i] >> 3)/31.0);
		//pixelArray[iX][iY].g = 0xFF*(( ((buffer[i] & 7) << 3) | (buffer[i+1] >> 5) )/63.0);
		//pixelArray[iX][iY].b = 0xFF*((buffer[i+1] & 31)/31.0);
		pixelArray[iX][iY].r = (buffer[i] & 0xF8);
		pixelArray[iX][iY].g = (((buffer[i] & 7) << 3) | (buffer[i+1] >> 5)) << 2;
		pixelArray[iX][iY].b = (buffer[i+1] & 31) << 3;
		/*switch (pixelArray[iX][iY].r+pixelArray[iX][iY].g+pixelArray[iX][iY].b){
			case 0:
				std::cout << " ";
				break;
				
			case 0xFF+0xFF+0xFF:
				std::cout << " ";
				break;

			default:
				std::cout << "#";
				break;
		}*/

		iX++;
		if (iX == 92){
			iX = 0;
			iY++;
			//std::cout << std::endl;
		}
	}

	//make pic
	writeBMP(imageOut, 92, 64, pixelArray);
	
	return 0;
}

int patchImage(char const* imageIn, char const* g3a, int address){
	//get pixel data
	pixel pixelArray[92][64];
	unsigned char pixelBuffer[92*64*3];
	FILE *fptr;
	fptr = fopen(g3a,"wb");
	unsigned int imageOffset;
	unsigned int headerSize;
	int width; int height;
	short int bitDepth;
	fseek ( fptr , 0x0A , SEEK_SET );
	fread(imageOffset, sizeof(unsigned char), 4, fptr);
	fread(headerSize, sizeof(unsigned char), 4, fptr);
	fread(width, sizeof(unsigned char), 4, fptr);
	fread(height, sizeof(unsigned char), 4, fptr);
	fseek ( fptr , 0x1C , SEEK_SET );
	fread(bitDepth, sizeof(unsigned char), 2, fptr);
	fseek ( fptr , imageOffset , SEEK_SET );
	fread(pixelBuffer, sizeof(unsigned char), 92*64*3, fptr);
	fclose(fptr);

	for (int x = 0; x < 92; x++){
		for (int y = 0; y < 64; y++){
			pixelArray[x][y].b = pixelBuffer[4 * (y * 92 + x)];
			pixelArray[x][y].g = pixelBuffer[4 * (y * 92 + x)+1];
			pixelArray[x][y].r = pixelBuffer[4 * (y * 92 + x)+2];
		}
	}

	writeBMP("TEST.bmp", 92, 64, pixelArray);

	//prepare buffer
	unsigned char buffer[0x2E00];
	int iX = 0;
	int iY = 0;
	for (int i = 0; i < 0x2E00; i+=2 ){
		unsigned char r = pixelArray[iX][iY].r >> 3;//((pixelArray[iX][iY].r/255.0)*31.0);
		unsigned char g = pixelArray[iX][iY].g >> 2;//((pixelArray[iX][iY].g/255.0)*63.0);
		unsigned char b = pixelArray[iX][iY].b >> 3;//((pixelArray[iX][iY].b/255.0)*31.0);

		buffer[i] = ((r << 3) | (g >> 3));
		buffer[i+1] = ((g << 5) | b);

		iX++;
		if (iX == 92){
			iX = 0;
			iY++;
			//std::cout << std::endl;
		}
	}

	//write file
	FILE *fptr;
	fptr = fopen(g3a,"wb");
	fseek ( fptr , address , SEEK_SET );
	int a = fwrite(buffer, sizeof(unsigned char), 0x2E00, fptr);
	std::cout << a << std::endl;
	fclose(fptr);

	return 0;
}

int main(int argc, char const *argv[])
{
	std::string selected = "selected.bmp";
	std::string unselected = "unselected.bmp";
	std::string g3a = "";
	bool extract = false;
	bool patch = false;
	//e for extract
	for (int i = 1; i < argc; i++){
		if(argv[i][0] == '-'){
			switch(argv[i][1]){
				case '-':
					//file name
					switch(argv[i][2]){
						case 's':
						//selected
							selected = argv[i+1];
						break;
						case 'u':
						//unselected
							unselected = argv[i+1];
						break;

						case 'i':
						//g3a
							g3a = argv[i+1];
						break;

						default:
							std::cout << "--" << argv[i][2] << " not recognised" << std::endl;
						break;
					}
				break;

				case 'e':
					//extract
					extract = true;
				break;

				case 'p':
					patch = true;
				break;

				default:
					std::cout << "-" << argv[i][1] << " not recognised" << std::endl;
				break;
			}
		}
	}

	if (g3a == ""){
		return -1;
	}else if (extract && patch){
		return -1;
	}else if (extract){
		extractImage(unselected.c_str(), g3a.c_str(), 0x1000);
		extractImage(selected.c_str(), g3a.c_str(), 0x4000);
	}else if (patch){
		patchImage(unselected.c_str(), g3a.c_str(), 0x1000); //have to run this twice or sdl makes image all blakc wtff.. doesnt matter writing our own bmp loader will fix this
		patchImage(unselected.c_str(), g3a.c_str(), 0x1000);
		patchImage(selected.c_str(), g3a.c_str(), 0x4000);
	}
	//extractImage(unselected, argv[1], 0x1000);
	//extractImage(selected, argv[1], 0x4000);
	//patchImage("first-extract.bmp", argv[1], 0x4000);
	//extractImage("second-extract.bmp", argv[1], 0x4000);
	return 0;
}