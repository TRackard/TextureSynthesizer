//"You can add spice to spaghetti, but it is still spaghetti" - Adithya Kumar


#include "defines.h"
#include <time.h> //for the random function
#include <math.h>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


//make setable by user later
double diff_tolerance = 0.35;


//Helper Function Prototypes
void copyToBuffer(uint8_t **sourceIMG, uint8_t **targetIMG, int width, int height, int channels);
void blendRegion(uint8_t **blendIMG, uint8_t **targetIMG, int width, int height, int channels, int offset);
double calDif(uint8_t **sourceIMG, uint8_t **compareIMG, int width, int height, int channels, int offset);
int findBestMatch(uint8_t **sourceIMG, uint8_t ***compareList, int num_in_list, int width, int height, int channels, int offset);

int main(int argc, char *argv[]){
	
	//Check user inputs
	if(argc < 4){
		printf("\nIncorrect number of arguments");
		printf("\nThe correct input format, goes as follows");
		printf("\n[input texture filename] [texton size] [output size]\n");
		printf("\ntexton size should be a multiple of 10\n");
		exit(EXIT_FAILURE);
	}
	
	//set vars to user inputs
	unsigned int texton = atoi(argv[2]);
	unsigned int outputSize = atoi(argv[3]);
	
	//error check user inputs
	if((texton % 2) != 0){
			printf("\nTexton should be a multiple of 2");
			printf("\nIt causes bugs with malloc ask for an odd amount of data");
			printf("\nExiting\n");
			exit(EXIT_FAILURE);
	}
	if((outputSize % 2) != 0){
			printf("\nOutput size should be a multiple of 2");
			printf("\nIt causes bugs with malloc ask for an odd amount of data");
			printf("\nExiting\n");
			exit(EXIT_FAILURE);
	}
	
	//Needed when generating the output image
	//Makes a buffer that is an integer multiple of the textons
	//It then copies that buffer to the output image
	unsigned int num_texton_out = (unsigned int)ceil((double)outputSize / (double)texton);
	unsigned int bufferSize = num_texton_out * texton;
	unsigned int bounds = num_texton_out * num_texton_out;
	
	//default values, for debugging
	int width = -1, height = -1, channels = -1;
	
	//uint8_t because the libray returns a pointer to the whole image
	//each element is 8 bits, rather each color is 8 bits
	uint8_t *buff = (uint8_t *)stbi_load(argv[1], &width, &height, &channels, STBI_rgb);
	
	//check to make sure it actually loaded
	if(buff == NULL){
		printf("\nCould not load texture.\nExiting program.");
		exit(EXIT_FAILURE);
	}
	
	//make sure that texton is within the image bounds
	if(width < texton || height < texton){
			printf("\nTexton is larger than the image dimensions\nExiting");
			exit(EXIT_FAILURE);
	}
	
	//seed the rand func to get "true" randomness
	srand(time(NULL));

	//==================================================================================================

	//potentially could be set by the user in the future
	//The number of blocks we randomly sample from the image
	//Also the offset when we compare overlapping regions
	unsigned int num_o_blocks = 10;
	unsigned int random = 0;
	int overlap = 4;

	//2D buffer for holding the randomly selected blocks
	uint8_t **block = NULL;
	
	//3D buffer that holds the blocks we randomly get
	uint8_t ***blockArray = malloc(num_o_blocks * sizeof(uint8_t **));
	if(blockArray == NULL){
		printf("\nCould not malloc space for blocks buffer!\nExiting program\n");
		exit(EXIT_FAILURE);
	}

	//using a goto statement in place of a for loop
	int blkNum = 0;
	repeat:

	//make a block the size of the texton
	block = (uint8_t **)malloc(texton * sizeof(uint8_t *));
	
	//get random index within boundaries of image
	random = rand() % (width - texton - overlap);
	
	//copy block set into buffer
	for(unsigned int h = 0; h < texton; h++){
		block[h] = (buff + (random * channels) + ((h + random) * width * channels));
	}

	//save block to buffer array
	blockArray[blkNum] = block;

	//call goto with if here
	//think of it as a less elegant do while loop
	if(blkNum < num_o_blocks){
		blkNum++;
		goto repeat;
	}
	
	//==================================================================================================


	//An ungodly amount of buffers
	uint8_t *image = (uint8_t *)malloc(bufferSize * (bufferSize * channels) * sizeof(uint8_t));
	uint8_t **imageArray = (uint8_t **)malloc(bufferSize * sizeof(uint8_t *));
	uint8_t *blockBufferBase = (uint8_t *)malloc(texton * texton * channels * sizeof(uint8_t));
	uint8_t **blockBuffer = (uint8_t **)malloc(texton * sizeof(uint8_t *));

	//Don't want to segfaults
	if(image == NULL || imageArray == NULL || blockBufferBase == NULL || blockBuffer == NULL) exit(EXIT_FAILURE);	

	//set up 2D array for easier access
	for(unsigned int h = 0; h < bufferSize; h++){
		imageArray[h] = (image + (h * bufferSize * channels));
	}

	
	//copy block set into buffer
	for(unsigned int h = 0; h < texton; h++){
		blockBuffer[h] = (blockBufferBase + (h * texton * channels));
	}


	

	unsigned int index = 0;
	unsigned int last_block = 0;
	unsigned int offset = 0;
	unsigned int offset_for_compare = texton;
	unsigned int compareHeight = texton;
	unsigned int compareWidth = 8;
	int counter = -1;
	
	
	//Copy blocks into the buffer for the output image
	for(unsigned int blk_index = 0; blk_index < bounds; blk_index++){
		
		//manages the offset for the output image
		//complex pointer BS and stuff
		if(((blk_index * texton) % bufferSize) == 0) counter++;
		
		//minus 1 prevents gaps between levels
		//plus the program doesn't segfault when I do that
		offset = counter * (texton - 1);
		
		//gets the index of the best match given the current block
		index = findBestMatch(blockArray[last_block], blockArray, num_o_blocks, compareHeight, compareWidth, channels, offset_for_compare);
		
		//copies it to a buffer as to not overwrite the original image data
		copyToBuffer(blockArray[index], blockBuffer, texton, texton, channels);
		
		//blends regions together
		//blendRegion(blockArray[last_block], blockBuffer, compareWidth, compareHeight, channels, offset_for_compare);
		
		//copy current block to the output buffer
		for(unsigned int h = 0; h < texton; h++){
			for(unsigned int w = 0; w < (texton * channels); w++){
				//complex pointer BS
				//Should have just done a 2D array
				*(image + w + ((h + offset) * bufferSize * channels) + (blk_index * texton * channels)) = blockBuffer[h][w];
			}
		}
		last_block = index;
	}

	
//==========================================================================================================
	//output buffer
	uint8_t *out = (uint8_t *)malloc(outputSize * (outputSize * channels) * sizeof(uint8_t));
	uint8_t **outArray = (uint8_t **)malloc(outputSize * sizeof(uint8_t *));

	if(out == NULL || outArray == NULL) exit(EXIT_FAILURE);	

	for(unsigned int h = 0; h < outputSize; h++){
		outArray[h] = (out + (h * outputSize * channels));
	}

	//copy relavent image data to buffer
	for(unsigned int h = 0; h < outputSize; h++){
		for(unsigned int w = 0; w < (outputSize * channels); w++){
			outArray[h][w] = imageArray[h][w];
		}
	}
	
	//finally write image to file
	if(stbi_write_bmp("out.bmp", outputSize, outputSize, channels, (void *)image) == 0){
		printf("\nCould not save texture\n");
	}



	//loop that frees each element in blockArray
	for(unsigned int loop = 0; loop < 10; loop++){
		free(blockArray[loop]);
	}
	//free for the array itself
	free(blockArray);

	free(image);
	free(imageArray);

	free(blockBufferBase);
	free(blockBuffer);
	
	//free image when done
	stbi_image_free(buff);

	return EXIT_SUCCESS;
}


//Helper functions
//===============================================================================================================
int findBestMatch(uint8_t **sourceIMG, uint8_t ***compareList, int num_in_list, int width, int height, int channels, int offset){
	int *indexArray = NULL;
	int num_within_tolerance = 0,
		index_of_smallest = 0;

	indexArray = (int *)malloc(num_in_list * sizeof(int));

	if(indexArray == NULL){
		exit(EXIT_FAILURE);
	}

	//loops through all blocks to find best match
	for(unsigned int blk_index = 0; blk_index < num_in_list; blk_index++){
		if(calDif(sourceIMG, compareList[blk_index], width, height, channels, offset) < diff_tolerance){
			indexArray[num_within_tolerance] = blk_index;
			num_within_tolerance++;
		}
	}

	//error checking if none are within tolerance
	//If not best match exists, get random
	if(num_within_tolerance == 0){
		index_of_smallest = rand() % num_in_list;
	}else{
		index_of_smallest = indexArray[rand() % num_within_tolerance];
	}

	//BE FREE MY MEMORY!!!
	free(indexArray);
	
	//give back index of best match
	return index_of_smallest;
}

//copies one block to another
void copyToBuffer(uint8_t **sourceIMG, uint8_t **targetIMG, int width, int height, int channels){
	for(uint32_t h = 0; h < height; h++){
		for(uint32_t w = 0; w < width * channels; w++){
			targetIMG[h][w] = sourceIMG[h][w];
		}
	}
}

//Simple blending function because I didn't have time to write a smarter one
void blendRegion(uint8_t **blendIMG, uint8_t **targetIMG, int width, int height, int channels, int offset){
	for(uint32_t h = 0; h < height; h++){
		for(uint32_t w = 0; w < width * channels; w++){
			targetIMG[h][w] = (blendIMG[h][w + offset] + targetIMG[h][w]) / 2;
		}
	}
}

//Because the library I used stores each individual color for each pixel contiguously,
//I opted to do a simple different between RGB values.
//Because for each pixel I can compare R with R, B with B, G with G
double calDif(uint8_t **sourceIMG, uint8_t **compareIMG, int width, int height, int channels, int offset){
	double dif = 0.0;
	offset *= channels;
	for(uint32_t h = 0; h < height; h++){
		for(uint32_t w = 0; w < width * channels; w++){
			dif += (double)abs(sourceIMG[h][w + offset] - compareIMG[h][w]);
		}
	}
	return (dif / (height * width * channels)) / 256.0;
}


