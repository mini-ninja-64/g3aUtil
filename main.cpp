#include <iostream>
#include <string>

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

(yn, x0)-->(y0,xn)
*/

struct pixel
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};
//92x64 24 bit bitmap header 
static const char BMP_HEADER_92_64[] = {0x42,0x4D,0x36,0x45,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,0x00,0x00,0x5C,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x45,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

//TODO: clean up all the things

int writeBMP(char const* filename, unsigned int width, unsigned int height, pixel imagePixels[92][64]) {
	unsigned char pixelBuffer[92*64*3];
	for (int x = 0; x < width; x++){
		for (int y = 0; y < height; y++){
			pixelBuffer[3 * (y * width + x)] = imagePixels[x][height-y-1].b;
			pixelBuffer[3 * (y * width + x)+1] = imagePixels[x][height-y-1].g;
			pixelBuffer[3 * (y * width + x)+2] = imagePixels[x][height-y-1].r;
		}
	}

	FILE *fptr;
	fptr = fopen(filename, "wb");
	fseek ( fptr , 0 , SEEK_SET );
	fwrite(BMP_HEADER_92_64, sizeof(unsigned char), 0x36, fptr);
	fwrite(pixelBuffer, sizeof(unsigned char), 92*64*3, fptr);
	fclose(fptr);
	//sstd::cout << SDL_GetPixelFormatName(surf->format->format) << std::endl;
	//SDL_SaveBMP(surf, filename);

	//SDL_FreeSurface(surf);

	//TODO: do fwrite and header euggh so much effort
	return 0;
}

int loadBMP(char const* filename) {

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
	FILE *fptr;
	fptr = fopen(imageIn,"rb");
	if (fptr == NULL){
		std::cerr << "Cannot load file" << std::endl;
	}
	unsigned int imageOffset;
	unsigned int headerSize;
	int width; int height;
	short int bitDepth;
	fseek ( fptr , 0x0A , SEEK_SET );
	fread(&imageOffset, sizeof(unsigned int), 1, fptr);
	fread(&headerSize, sizeof(unsigned int), 1, fptr);
	fread(&width, sizeof(int), 1, fptr);
	fread(&height, sizeof(int), 1, fptr);
	fseek ( fptr , 0x1C , SEEK_SET );
	fread(&bitDepth, sizeof(short int), 1, fptr);
	fseek ( fptr , imageOffset , SEEK_SET );

	float rowSize = 4.0*((bitDepth * width + 31.0)/32.0);

	//error checking
	if ((width != 92) || (height != 64)){
		std::cerr << "Image must be 92x64 bitmap" << std::endl;
		return -1;
	}

	if (bitDepth != 24){
		std::cerr << "Image must be 24 bit bitmap" << std::endl;
		return -1;
	}

	//get pixel data
	pixel pixelArray[92][64];
	int padding = (width*bitDepth) % 4;
	unsigned char pixelBuffer[92*64*3+(92*padding)];

	fread(pixelBuffer, sizeof(unsigned char), 92*64*3+(92*padding), fptr);
	fclose(fptr);

	std::cout << width << ", " << height << " @ " << bitDepth << std::endl << "Offset: " << imageOffset << std::endl << std::hex << (int)pixelBuffer[0] <<  ", " << (int)pixelBuffer[1] << ", " << (int)pixelBuffer[2] << std::endl << "Row Size: " << rowSize << std::endl << "Padding: " << padding << std::endl;

	for (int x = 0; x < 92; x++){
		for (int y = 0; y < 64; y++){
			pixelArray[x][height-y-1].b = pixelBuffer[3 * (y * width + x)];
			pixelArray[x][height-y-1].g = pixelBuffer[3 * (y * width + x)+1];
			pixelArray[x][height-y-1].r = pixelBuffer[3 * (y * width + x)+2];
		}
	}

	//writeBMP("TEST.bmp", 92, 64, pixelArray);

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
	fptr = fopen(g3a,"rb+");
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
							std::cout << "Argument --" << argv[i][2] << " not recognised" << std::endl;
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
					std::cout << "Argument -" << argv[i][1] << " not recognised" << std::endl;
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
		patchImage(unselected.c_str(), g3a.c_str(), 0x1000);
		patchImage(selected.c_str(), g3a.c_str(), 0x4000);
	}
	return 0;
}