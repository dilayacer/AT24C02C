/*
 * eeprom.c
 *
 *  Created on: Mar 10, 202x
 *      Author: dilayacer
 */

#include "eeprom.h"
#include "math.h"
#include "string.h"

/* USER DEFINITIONS.............................................................................................*/
#define EEPROM_I2C &hi2c1
#define EEPROM_ADDR 0xA0   //EEPROM Address
#define MEM_ADDR_SIZE 1    //EEPROM Memory Address Size 1 Byte
#define PAGE_SIZE 8		   //EEPROM Page Size 8 Byte
#define PAGE_NUM  32      //EEPROM Page Number 32
#define BYTES_TO_GO(size, offset) size+offset<PAGE_SIZE?size:PAGE_SIZE-offset; //Calculates Remaining Bytes for EEPROM R/W

extern I2C_HandleTypeDef hi2c1;
extern CRC_HandleTypeDef hcrc;

//arrays
uint8_t arrayW[100]; //Array required for bulk writing to EEPROM. All variables are kept in this array.
uint8_t arrayR[100]; //Array required for reading to EEPROM. All variables are kept in this array.
uint8_t arrayCRC[8]; //Array where data is held to write and read the CRC value
uint8_t arrayZ[8]; //Array used to check the correctness of data after writing data individual
uint8_t buf2[8]; //We keep pages in buf2 to read CRC
uint8_t buf[8];  //We keep pages in buf to read CRC

//variable
const int PAGE_SIZE_EXPONENT = log(PAGE_SIZE)/log(2); //Start bit of page address = 4 ref. datasheet
uint32_t retval_uint; //As a result of single uint writing, the data written is read and assigned to this variable.
float retval_float; //As a result of single float writing, the data written is read and assigned to this variable.
uint16_t calcCrc; //Calculated CRC value

float kp, ki, kd, overTemp;
uint16_t hiz;
uint32_t peakCrrnt, contCrrnt;

//Initial values are given. These values are saved in the EEPROM.
///*
float kp_i = 412.5;
float ki_i = 417.3;
float kd_i = 412.6;
float overTemp_i = 412.7;
uint32_t peakCrrnt_i = 4810;
uint32_t contCrrnt_i = 1170;
uint16_t hiz_i = 111;
//*/

//These variables will come from the motor driver
float kp_s = 115.5;
float ki_s = 116.5;
float kd_s = 116.7;
float overTemp_s = 113.3;
uint32_t peakCrrnt_s = 118;
uint32_t contCrrnt_s = 119;
uint16_t hiz_s = 117;
uint8_t val=0;

/* EEPROM WRITE/READ/ERASE FUNCTIONS.....................................................................*/
void eeprom_Write (uint16_t page, uint16_t offset, uint8_t *data, uint16_t size) //EEPROM Write Func.
{
	uint16_t startPage = page;  //The start page is calculated
	if (size <= 0) return;
	uint16_t endPage = page + ((size + offset - 1) >> PAGE_SIZE_EXPONENT); //The end page is calculated
	uint16_t bytepos=0;
	for (; startPage <= endPage; startPage++)
	{
		uint16_t MemAddress = startPage<<PAGE_SIZE_EXPONENT | offset; //The eeprom memory address is specified according to the page and offset entered as a parameter.
		uint16_t bytestowrite = BYTES_TO_GO(size, offset); //Calculates the data size by offset, keeps the incremental number of data for the other page.
		HAL_I2C_Mem_Write(EEPROM_I2C, EEPROM_ADDR, MemAddress, MEM_ADDR_SIZE,  &data[bytepos], bytestowrite, 1000); //EEPROM write with Hal lib
		offset = 0; //If there is an offset, the offset is reset so that the data is written to the other pages from the 0th index.
		size -= bytestowrite; //The number of data to be written for the other page is reduced from the size variable.
		bytepos += bytestowrite;
		HAL_Delay (5); //EEPROM needs 5ms write time ref. datasheet
	}
}

void eeprom_Read (uint16_t page, uint16_t offset, uint8_t *data, uint16_t size) //EEPROM Read Func.
{
	uint16_t startPage = page;
	if (size <= 0) return;
	uint16_t endPage = page + ((size + offset - 1) >> PAGE_SIZE_EXPONENT);
	uint16_t bytepos=0;
	for (; startPage <= endPage; startPage++)
	{
		uint16_t MemAddress = startPage<<PAGE_SIZE_EXPONENT | offset;
		uint16_t bytestoread = BYTES_TO_GO(size, offset);
		HAL_I2C_Mem_Read(EEPROM_I2C, EEPROM_ADDR, MemAddress, MEM_ADDR_SIZE,  &data[bytepos], bytestoread, 1000); //EEPROM read with Hal lib
		offset = 0;
		size -= bytestoread;
		bytepos += bytestoread;
	}
}

void eeprom_Erase (uint16_t page) //EEPROM Erase Func.
{
	uint16_t MemAddress = page<<PAGE_SIZE_EXPONENT;
	uint8_t data[PAGE_SIZE];
	memset(data, 0xFF ,PAGE_SIZE); //0xFF value is written to memory with memset
	HAL_I2C_Mem_Write(EEPROM_I2C, EEPROM_ADDR, MemAddress, MEM_ADDR_SIZE, data, PAGE_SIZE, 1000);
	HAL_Delay (5);
}

void clear_all_eeprom()  //All EEPROM must be cleaned before use
{
	 for(int i=0; i<32; i++)
	  {
	 	  eeprom_Erase(i);
	  }
	  eeprom_Read(0, 0, arrayR, 100);
}

/* BIT ARRAY CONVERT FUNCTIONS FOR WRITE......................................................................*/
void bitArray_uint32_to_uint8(uint32_t value, uint8_t *array, uint8_t index) { //converts from uint32 to uint8 //write_uint32_to_uint8
    array[index] = (value & 0xFF);
    array[index + 1] = ((value >> 8) & 0xFF);
    array[index + 2] = ((value >> 16) & 0xFF);
    array[index + 3] = ((value >> 24) & 0xFF);
}

void bitArray_uint16_to_uint8(uint16_t value, uint8_t *array, uint8_t index) { //converts from uint16 to uint8 //write_uint16_to_uint8
    array[index] = (value & 0xFF);
    array[index + 1] = ((value >> 8) & 0xFF);
}

void bitArray_float_to_uint8(float value, uint8_t *array, uint8_t index) { //converts from float to uint8 //write_float_to_uint8
    union {
        float f;
        uint32_t u;
    } float_converter;

    float_converter.f = value;

    array[index] = (float_converter.u & 0xFF);
    array[index + 1] = ((float_converter.u >> 8) & 0xFF);
    array[index + 2] = ((float_converter.u >> 16) & 0xFF);
    array[index + 3] = ((float_converter.u >> 24) & 0xFF);
}

void writeCRC_MSB(uint16_t value, uint8_t *array, uint8_t index) { //Writes the CRC value to the array according to the MSB //write_crc_msb
    array[index] = ((value >> 8) & 0xFF);
    array[index + 1] = (value & 0xFF);
}

/* BIT ARRAY CONVERT FUNCTIONS FOR READ....................................................................*/
uint32_t bitArray_uint8_to_uint32(uint8_t *array, uint8_t index) { //converts from uint8 to uint32 //read_uint32_to_uint8
    uint32_t value = 0;
    value |= array[index];
    value |= array[index + 1] << 8;
    value |= array[index + 2] << 16;
    value |= array[index + 3] << 24;
    return value;
}

uint16_t bitArray_uint8_to_uint16 (uint8_t *array, uint8_t index) { //converts from uint8 to uint16 //read_uint16_to_uint8
    uint16_t value = 0;
    value |= array[index];
    value |= array[index + 1] << 8;
    return value;
}

float bitArray_uint8_to_float(uint8_t *array, uint8_t index) { //converts from uint8 to float //read_float_from_uint8
    union FloatUnion {
        float f;
        uint8_t bytes[4];
    } data;

    data.bytes[0] = array[index];
    data.bytes[1] = array[index + 1];
    data.bytes[2] = array[index + 2];
    data.bytes[3] = array[index + 3];

    return data.f;
}

/* INITIAL WRITE AND READ .......................................................................*/
uint32_t init_Read_Fill()
{
	/*
	  bitArray_float_to_uint8(kp_i, arrayW, 0);
	  calcCrc = HAL_CRC_Calculate(&hcrc, arrayW, 6);
	  writeCRC_MSB(calcCrc, arrayW, 6);

	  bitArray_float_to_uint8(ki_i, arrayW, 8);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+8) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 14);

	  bitArray_float_to_uint8(kd_i, arrayW, 16);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+16) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 22);

	  bitArray_float_to_uint8(overTemp_i, arrayW, 24);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+24) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 30);

	  bitArray_uint32_to_uint8(peakCrrnt_i, arrayW, 32);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+32) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 38);

	  bitArray_uint32_to_uint8(contCrrnt_i, arrayW, 40);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+40) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 46);

	  bitArray_uint16_to_uint8(hiz_i, arrayW, 48);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+48) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 54);

	  eeprom_Write(0, 0, arrayW, sizeof(arrayW));
*/
	  for(int i = 0; i<10; i++)
	{
	  eeprom_Read(i, 0, buf, sizeof(buf));
	  val = HAL_CRC_Calculate(&hcrc, buf, sizeof(buf));
	  if(val!=0)
		  return -1;
	}

	  eeprom_Read(0, 0, arrayR, 100);

	  if(val==0) {
	  kp = bitArray_uint8_to_float(arrayR, 0);
	  ki= bitArray_uint8_to_float(arrayR, 8);
	  kd= bitArray_uint8_to_float(arrayR, 16);
	  overTemp = bitArray_uint8_to_float(arrayR, 24);
	  peakCrrnt = bitArray_uint8_to_uint32(arrayR, 32);
	  contCrrnt = bitArray_uint8_to_uint32(arrayR, 40);
	  hiz = bitArray_uint8_to_uint16(arrayR, 48);
	  }
}

/* WRITE ALL DATA .......................................................................*/

uint32_t write_Flash()
{
	  bitArray_float_to_uint8(kp_s, arrayW, 0);
	  calcCrc = HAL_CRC_Calculate(&hcrc, arrayW, 6);
	  writeCRC_MSB(calcCrc, arrayW, 6);

	  bitArray_float_to_uint8(ki_s, arrayW, 8);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+8) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 14);

	  bitArray_float_to_uint8(kd_s, arrayW, 16);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+16) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 22);

	  bitArray_float_to_uint8(overTemp_s, arrayW, 24);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+24) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 30);

	  bitArray_uint32_to_uint8(peakCrrnt_s, arrayW, 32);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+32) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 38);

	  bitArray_uint32_to_uint8(contCrrnt_s, arrayW, 40);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+40) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 46);

	  bitArray_uint16_to_uint8(hiz_s, arrayW, 48);
	  calcCrc = HAL_CRC_Calculate(&hcrc, (arrayW+48) , 6);
	  writeCRC_MSB(calcCrc, arrayW, 54);

	  eeprom_Write(0, 0, arrayW, sizeof(arrayW));

	  for(int i = 0; i<10; i++)
	  {
		  eeprom_Read(i, 0, buf2, sizeof(buf2));
		  val = HAL_CRC_Calculate(&hcrc, buf2, sizeof(buf2));
		  if(val!=0)
			  return -1;
	  }
	  eeprom_Read(0, 0, arrayR, 100);

	  if(val==0) {
	  kp = bitArray_uint8_to_float(arrayR, 0);
	  ki= bitArray_uint8_to_float(arrayR, 8);
	  kd= bitArray_uint8_to_float(arrayR, 16);
	  overTemp = bitArray_uint8_to_float(arrayR, 24);
	  peakCrrnt = bitArray_uint8_to_uint32(arrayR, 32);
	  contCrrnt = bitArray_uint8_to_uint32(arrayR, 40);
	  hiz = bitArray_uint8_to_uint16(arrayR, 48);
	  }
}

/* FUNCTION TO READ VARIABLES INDIVIDUALLY............................................................*/
uint32_t singleRead_uint(uint8_t a, uint8_t b, uint32_t *result) //The data of type uint is read, the value read is transferred to the "result". //crc_read_data_uint
{
    uint8_t first = a % 8; //mod operation is done to find the first byte
    uint8_t last = b % 8; //mod operation is done to find the last byte

    uint16_t current_page = (a - first) / 8; //find current page
    if (b / 8 > current_page) {
        current_page = b / 8;
    }

    eeprom_Read(current_page, 0, arrayCRC, sizeof(arrayCRC)); //8 bytes of the current page are written to arrayCRCR
    calcCrc = HAL_CRC_Calculate(&hcrc, arrayCRC, 8); //calculating CRC for arrayCRCR

    uint32_t value_uint = 0;
    if (calcCrc != 0) //Returns an error if the CRC value is not equal to 0
    {
    	return -1;
    }
    else
    {
        for (first; first < last; first++) //It is read between the entered bytes. Values read as bit arrays are converted to uint32.
        {
        	value_uint |= arrayCRC[first*1] << ((first - a % 8) * 8);
        }
    }
     result = &value_uint; //The result is assigned to the "result" parameter.
}


uint32_t singleRead_float(uint8_t a, uint8_t b, float *result) //The data of type float is read, the value read is transferred to the "result". //crc_read_data_float
{
    uint8_t first = a % 8; //mod operation is done to find the first byte
    uint8_t last = b % 8; //mod operation is done to find the last byte

    uint16_t current_page = (a - first) / 8; //find current page
    if (b / 8 > current_page) {
        current_page = b / 8;
    }

    eeprom_Read(current_page, 0, arrayCRC, sizeof(arrayCRC)); //8 bytes of the current page are written to arrayCRCR
    calcCrc = HAL_CRC_Calculate(&hcrc, arrayCRC, 8); //calculating CRC for arrayCRCR

    float value_float = 0;
    if (calcCrc != 0) //Returns an error if the CRC value is not equal to 0
    {
    	return -1;
    }
    else
    {
        union FloatUnion { //It is read between the entered bytes. Values read as bit arrays are converted to float.
            float f;
            uint8_t bytes[4];
        } data;
        for (first; first<last; first++) {
            data.bytes[first*1] = arrayCRC[first*1];
        }
        value_float = data.f;
    }
    result = &value_float; //The result is assigned to the "result" parameter.
}

/* FUNCTION TO WRITE VARIABLES INDIVIDUALLY....................*/
void clearByte(uint16_t a, uint16_t b) //When we change the variables, we clear the EEPROM so that it does not overwrite //kalan
{
	uint8_t first = a%8;  //mod operation is done to find the first byte
	uint8_t data[4] = {0}; //A 4-byte array is created. The array contains 0.
    uint16_t current_page = (a - first) / 8; //find current page
    if (b / 8 > current_page) {
        current_page = b / 8;
    }
    if(first==0) { //If the first byte is equal to 0, the first 4 bytes of the current page in the EEPROM are set to 0. This is done to avoid overwriting.
    	eeprom_Write(current_page, 0, data, 4);
    }
    if(first==4) {
    	eeprom_Write(current_page, 4, data, 2);
    }
}
//singleWrite_float crc_write_float
uint8_t singleWrite_float(uint16_t a, uint16_t b, float data_float) //Writes the float value of the data with data_float to the bytes a and b
{
	uint8_t first = a%8; //mod operation is done to find the first byte
	uint8_t last = b%8; //mod operation is done to find the last byte
	uint8_t size = b-a; //Find the number of bytes to change

    uint16_t current_page = (a - first) / 8; //find current page
    if (b / 8 > current_page) {
        current_page = b / 8;
    }
	eeprom_Read(current_page, 0, arrayCRC, sizeof(arrayCRC)); //The current page is read from the EEPROM and written to the arrayCRCR array.
	calcCrc = HAL_CRC_Calculate(&hcrc, arrayCRC, 8); //calculating CRC
	if(calcCrc!=0){ //Returns an error if the CRC value is not equal to 0
		return -1;
	}
	else{
	     clearByte(a,b); //Bytes to be written are cleared
		 eeprom_Read(0, 0, arrayR, 100); //EEPROM read
		 bitArray_float_to_uint8(data_float, arrayCRC, first); //The data to be added is converted into bit array format.
		 eeprom_Write(current_page, 0, arrayCRC, size); //The size of the data is written for the arrayCRCR array
		 calcCrc = HAL_CRC_Calculate(&hcrc, arrayCRC, 6); //Calculate the CRC value of the first 6 bytes of this array
		 writeCRC_MSB(calcCrc, arrayCRC, 6); //The calculated CRC value is written into the array CRC array
		 eeprom_Write(current_page, 0, arrayCRC, 8); //arrayCRC array (data and CRC value) is written to EEPROM
		 eeprom_Read(0, 0, arrayR, 100); //Read from EEPROM
		 calcCrc = HAL_CRC_Calculate(&hcrc, arrayCRC, 8); //CRC value is calculated again

		 if(calcCrc == 0) { //If the calculated CRC value is equal to 0, the data is written correctly
		 eeprom_Read(current_page, first, arrayZ, size); //Re-reading is done to check the accuracy of the written data.
		 retval_float = bitArray_uint8_to_float(arrayZ, 0); //The value read is assigned in retval_float.
		 }
	}
}

//singleWrite_uint crc_write_uint
uint8_t singleWrite_uint(uint16_t a, uint16_t b, uint32_t data_uint) //Writes the uint value of the data with data to the bytes a and b
{
	uint8_t first = a%8;
	uint8_t last = b%8;
	uint8_t size = b-a;
    uint16_t current_page = (a - first) / 8;
    if (b / 8 > current_page) {
        current_page = b / 8;
    }
	eeprom_Read(current_page, 0, arrayCRC, sizeof(arrayCRC));
	calcCrc = HAL_CRC_Calculate(&hcrc, arrayCRC, 8);
	if(calcCrc!=0){
		return -1;
	}
	else{
		 clearByte(a,b);
		 eeprom_Read(0, 0, arrayR, 100);
		 bitArray_uint32_to_uint8(data_uint, arrayCRC, first);
		 eeprom_Write(current_page, 0, arrayCRC, size);
		 calcCrc = HAL_CRC_Calculate(&hcrc, arrayCRC, 6);
		 writeCRC_MSB(calcCrc, arrayCRC, 6);
		 eeprom_Write(current_page, 0, arrayCRC, 8);
		 eeprom_Read(0, 0, arrayR, 100);
		 calcCrc = HAL_CRC_Calculate(&hcrc, arrayCRC, 8);

		 if(calcCrc==0){
		 eeprom_Read(current_page, first, arrayZ, size);
		 retval_uint = bitArray_uint8_to_uint32(arrayZ, 0);//The value read is assigned in retval_uint.
		 }
	}
}

