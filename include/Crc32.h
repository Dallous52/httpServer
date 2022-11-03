#ifndef Crc32
#define Crc32

#include <stdint.h>

extern "C" 
{
	//Crc32У��ֵ���㴦����
	uint32_t crc32(const unsigned char* s, unsigned int len);

	//Crc32����ʽУ�麯��
	uint32_t crc32_poly(uint32_t poly, uint16_t value);
}

#endif // !Crc32

