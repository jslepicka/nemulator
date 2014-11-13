#include <Windows.h>

unsigned int get_crc32(const unsigned char *buf, unsigned int len)
{
	 static unsigned int table[256];
	 static long table_built = 0;
	 unsigned int rem;
	 unsigned int octet;

	 if (InterlockedCompareExchange(&table_built, 1, 0) == 0)
	 {
		 for (int i = 0; i < 256; i++)
		 {
			 rem = i;
			 for (int j = 0; j < 8; j++)
			 {
				 if (rem & 0x1)
				 {
					 rem >>= 1;
					 rem ^= 0xedb88320;
				 }
				 else
				 {
					 rem >>= 1;
				 }
			 }
			 table[i] = rem;
		 }
	 }

	 unsigned int crc = ~0;
	 for (const unsigned char *p = buf; p < buf + len; p++)
	 {
		 octet = *p;
		 crc = (crc >> 8) ^ table[(crc & 0xFF) ^ octet];
	 }
	 return ~crc;
}