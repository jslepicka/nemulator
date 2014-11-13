#pragma once
class c_nes;
class Nes_Apu;
class Blip_Buffer;

class c_apu
	{
	public:
		c_apu(void);
		~c_apu(void);
		unsigned char ReadByte(unsigned short address);
		void WriteByte(unsigned short address, unsigned char value);
		int EndFrame(void);
		void Reset(void);
		short *output;
		int samples;
		c_nes *nes;
		void EndLine(int cycles);
	private:
		static const int baserate = 1789773;
		static const int sampleBufferSize = 131072;
		Nes_Apu *apu;
		Blip_Buffer *buf;
	};