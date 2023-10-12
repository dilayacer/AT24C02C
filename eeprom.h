/*
 * eeprom.h
 *
 *  Created on: Mar 10, 202x
 *      Author: dilayacer
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include "stdint.h"
#include "stm32g4xx_hal.h"

//EEPROM FUNCTIONS
void eeprom_Write (uint16_t page, uint16_t offset, uint8_t *data, uint16_t size);
void eeprom_Read (uint16_t page, uint16_t offset, uint8_t *data, uint16_t size);
void eeprom_Erase (uint16_t page);
void clear_all_eeprom();

//BIT ARRAY WRITE FUNCTION
void bitArray_uint32_to_uint8(uint32_t value, uint8_t *array, uint8_t index);
void bitArray_uint16_to_uint8(uint16_t value, uint8_t *array, uint8_t index);
void bitArray_float_to_uint8(float value, uint8_t *array, uint8_t index);
void writeCRC_MSB(uint16_t value, uint8_t *array, uint8_t index);

//BIT ARRAY READ FUNCTIONS
uint32_t bitArray_uint8_to_uint32(uint8_t *array, uint8_t index);
uint16_t bitArray_uint8_to_uint16 (uint8_t *array, uint8_t index);
float bitArray_uint8_to_float(uint8_t *array, uint8_t index);

//INITIAL FUNCTIONS
uint32_t init_Read_Fill();
uint32_t write_Flash();

//FUNCTION TO READ VARIABLES INDIVIDUALLY
uint32_t singleRead_uint(uint8_t a, uint8_t b, uint32_t *sonuc);
uint32_t singleRead_float(uint8_t a, uint8_t b, float *sonuc);

//FUNCTION TO WRITE VARIABLES INDIVIDUALLY
void clearByte(uint16_t a, uint16_t b);
uint8_t singleWrite_float(uint16_t a, uint16_t b, float data_f);
uint8_t singleWrite_uint(uint16_t a, uint16_t b, uint32_t data);

#endif /* INC_EEPROM_H_ */
