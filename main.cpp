#include <iostream>
#include <SDL2/SDL.h>
//using rgb 565
/*
standardising values
R: 0xFF*(first 5 bits/31)
G: 0xFF*(bit 6 to 11/63)
B: 0xFF*(bit 12 to 16/31)

*/

struct pixel
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

int writeBMP(char* filename, unsigned int width, unsigned int height, pixel imagePixels[92][64]) {
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
	SDL_SaveBMP(surf, filename);
	SDL_FreeSurface(surf);
	return 0;
}

int main(int argc, char const *argv[])
{
	unsigned char unselectedBuffer[0x2E00];
	unsigned char selectedBuffer[0x2E00];
	pixel unselectedPixelArray[92][64];
	pixel selectedPixelArray[92][64];

	FILE *fptr;
	fptr = fopen(argv[1],"rb");
	fseek ( fptr , 0x1000 , SEEK_SET );
	fread(unselectedBuffer, sizeof(unsigned char), 0x2E00, fptr);
	fseek ( fptr , 0x4000 , SEEK_SET );
	fread(selectedBuffer, sizeof(unsigned char), 0x2E00, fptr);
	fclose(fptr);

	int iX = 0;
	int iY = 0;
	for (int i = 0; i < 0x2E00; i+=2 ){
		selectedPixelArray[iX][iY].r = 0xFF*((selectedBuffer[i] >> 3)/31.0);
		selectedPixelArray[iX][iY].g = 0xFF*(( ((selectedBuffer[i] & 7) << 3) | (selectedBuffer[i+1] >> 5) )/63.0);
		selectedPixelArray[iX][iY].b = 0xFF*((selectedBuffer[i+1] & 31)/31.0);
		unselectedPixelArray[iX][iY].r = 0xFF*((unselectedBuffer[i] >> 3)/31.0);
		unselectedPixelArray[iX][iY].g = 0xFF*(( ((unselectedBuffer[i] & 7) << 3) | (unselectedBuffer[i+1] >> 5) )/63.0);
		unselectedPixelArray[iX][iY].b = 0xFF*((unselectedBuffer[i+1] & 31)/31.0);
		switch (unselectedPixelArray[iX][iY].r+unselectedPixelArray[iX][iY].g+unselectedPixelArray[iX][iY].b){
			case 0:
				std::cout << " ";
				break;
				
			case 0xFF+0xFF+0xFF:
				std::cout << " ";
				break;

			default:
				std::cout << "#";
				break;
		}

		iX++;
		if (iX == 92){
			iX = 0;
			iY++;
			std::cout << std::endl;
		}
	}

	//make pic
	writeBMP("selected.bmp", 92, 64, selectedPixelArray);
	writeBMP("unselected.bmp", 92, 64, unselectedPixelArray);

	std::cout << "^^^^Preview^^^^" << std::endl;
	return 0;
}