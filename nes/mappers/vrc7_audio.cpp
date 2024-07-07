
#include <cmath>
#include <algorithm>
#include "vrc7_audio.h"
#include <stdint.h>

//#define ONE_CHANNEL_ONLY        1
//#define NO_MODULATOR


namespace
{
    const int           ClockDiv = 36;
    const uint32_t      MaxAtten = (1<<23);

    const uint8_t FixedInstrumentTable[0x80] = 
    {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // custom instrument
        0x03,0x21,0x04,0x06,0x8D,0xF2,0x42,0x17,  // begin fixed instruments
        0x13,0x41,0x05,0x0E,0x99,0x96,0x63,0x12,
        0x31,0x11,0x10,0x0A,0xF0,0x9C,0x32,0x02,
        0x21,0x61,0x1D,0x07,0x9F,0x64,0x20,0x27,
        0x22,0x21,0x1E,0x06,0xF0,0x76,0x08,0x28,
        0x02,0x01,0x06,0x00,0xF0,0xF2,0x03,0x95,
        0x21,0x61,0x1C,0x07,0x82,0x81,0x16,0x07,
        0x23,0x21,0x1A,0x17,0xEF,0x82,0x25,0x15,
        0x25,0x11,0x1F,0x00,0x86,0x41,0x20,0x11,
        0x85,0x01,0x1F,0x0F,0xE4,0xA2,0x11,0x12,
        0x07,0xC1,0x2B,0x45,0xB4,0xF1,0x24,0xF4,
        0x61,0x23,0x11,0x06,0x96,0x96,0x13,0x16,
        0x01,0x02,0xD3,0x05,0x82,0xA2,0x31,0x51,
        0x61,0x22,0x0D,0x02,0xC3,0x7F,0x24,0x05,
        0x21,0x62,0x0E,0x00,0xA1,0xA0,0x44,0x17
    };

    inline int dB(double dB)
    {
        return static_cast<int>(dB * MaxAtten / 48.0);
    }

    ////  half sine table
    //  phase generator is 18 bits wide.  High bit 17 is used for rectification
    // so that means the index can be any number of bits 16:0
    const int       HalfSineBits = 10;
    const int       HalfSineWidth = (1<<HalfSineBits);
    const int       PhaseShift = 17 - HalfSineBits;
    const int       PhaseMask = HalfSineWidth - 1;
    uint32_t HalfSineLut[HalfSineWidth];

    //// Attack rate table
    //  envelope generator is 23 bits wide.  Attack rate table can be any number
    // of bits up to that
    const int       AttackLutBits = 8;      // no higher than 23
    const int       AttackLutWidth = (1<<AttackLutBits);
    const int       AttackLutShift = 23-AttackLutBits;
    const int       AttackLutMask = AttackLutWidth - 1;
    uint32_t AttackLut[AttackLutWidth];

    //// dB to Linear table
    //  envelope levels are 23 bits wide, based in dB
    //  channel output is 20 bits wide, linear
    const int       LinearLutBits = 16;
    const int       LinearLutWidth = (1 << LinearLutBits);
    const int       LinearLutShift = 23 - LinearLutBits;
    int LinearLut[LinearLutWidth];

    //// AM / Tremolo table
    //  am rate accumulator is 20 bits wide
    const int       AmLutBits = 8;
    const int       AmLutWidth = (1 << AmLutBits);
    const int       AmLutShift = 20 - AmLutBits;
    const int       AmLutMask = (AmLutWidth - 1);
    uint32_t AmLut[AmLutWidth];
    
    //// FM / Vibratto table
    //  fm rate accumulator is 20 bits wide
    const int       FmLutBits = 8;
    const int       FmLutWidth = (1 << FmLutBits);
    const int       FmLutShift = 20 - FmLutBits;
    const int       FmLutMask = (FmLutWidth - 1);
    double FmLut[FmLutWidth];


    bool TablesBuilt = false;
    void BuildTables()
    {
        static const double pi = 3.1415926535897932384626433832795;

        // Build the half sine lut
        for(int i = 0; i < HalfSineWidth; ++i)
        {
            static const double scale = MaxAtten / 48.0;

            double sinx = sin( pi * i / HalfSineWidth );
            if(!sinx)       HalfSineLut[i] = MaxAtten;
            else
            {
                sinx = -20.0 * log10(sinx) * scale;
                if(sinx > MaxAtten) HalfSineLut[i] = MaxAtten;
                else                HalfSineLut[i] = static_cast<uint32_t>(sinx);
            }
        }

        // Build the Attack Rate Lut
        for(int i = 0; i < AttackLutWidth; ++i)
        {
            static const double baselog = log( static_cast<double>( MaxAtten >> AttackLutShift ) );

            AttackLut[i] = dB(48) - static_cast<uint32_t>( dB(48) * log(static_cast<double>(i)) / baselog );
        }

        // dB to Linear Lut
        for(int i = 0; i < LinearLutWidth; ++i)
        {
            static const int outscale = (1 << 20);              // channel output is 20 bits wide
            static const double inscale = MaxAtten / 48.0 / (1 << LinearLutShift);

            double lin = pow(10.0, (i / -20.0 / inscale));
            LinearLut[i] = static_cast<int>(lin * outscale);
        }

        // Am table
        for(int i = 0; i < AmLutWidth; ++i)
        {
            AmLut[i] = static_cast<uint32_t>( (1.0 + sin(2 * pi * i / AmLutWidth)) * dB(0.6) );
        }

        // Fm table
        for(int i = 0; i < FmLutWidth; ++i)
        {
            FmLut[i] = pow(2.0, 13.75 / 1200 * sin(2 * pi * i / FmLutWidth));
        }
    }
}//anon namespace

namespace schpune
{

void Vrc7Audio::Reset(bool hard)
{
    if(!TablesBuilt)
        BuildTables();

    mAmUnit.Reset(hard);
    mFmUnit.Reset(hard);
    
    std::copy(FixedInstrumentTable,FixedInstrumentTable + 0x80,mInstData);

    for(int i = 0; i < 6; ++i)
        mChannel[i].Reset(hard, mInstData, &mAmUnit, &mFmUnit);

    mTicksToNextClock = ClockDiv;
    mAddr = 0;
}

void Vrc7Audio::Write(uint16_t a, uint8_t v)
{
    switch(a)
    {
    case 0x9010:    mAddr = v & 0x3F;       break;
    case 0x9030:
        switch(mAddr)
        {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
            mInstData[mAddr] = v;
            for(int i = 0; i < 6; ++i)
                mChannel[i].RefreshInstZero();
            break;
            
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
        case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:
#ifdef ONE_CHANNEL_ONLY
            if((mAddr & 7) == ONE_CHANNEL_ONLY)
#endif
            mChannel[mAddr & 7].Write(mAddr, v);
            break;
        }
        break;
    }
}

int Vrc7Audio::clock( int ticks )
{
    mTicksToNextClock -= ticks;
    //while(mTicksToNextClock < 0)
    //{
        mAmUnit.Clock();
        mFmUnit.Clock();

        int out = 0;
        for(int i = 0; i < 6; ++i)
        {
#ifdef ONE_CHANNEL_ONLY
            if(i == ONE_CHANNEL_ONLY)
#endif
            out += mChannel[i].Clock();
        }

        out >>= 7;      // TODO scale this?
        if(out != mPrevOut)
        {
            mPrevOut = out;
        }

        mTicksToNextClock += ClockDiv;
   /* }*/
        audio_out = out;
        return out;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

void Vrc7Audio::AmUnit::Reset(bool hard)
{
    mPhase = 0;
    mOut = 0;
}

void Vrc7Audio::AmUnit::Clock()
{
    mPhase += 78;
    mOut = AmLut[ (mPhase >> AmLutShift) & AmLutMask ];
}

void Vrc7Audio::FmUnit::Reset(bool hard)
{
    mPhase = 0;
    mOut = 0;
}

void Vrc7Audio::FmUnit::Clock()
{
    mPhase += 105;
    mOut = FmLut[ (mPhase >> FmLutShift) & FmLutMask ];
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

void Vrc7Audio::Slot::Reset(bool hard,int iscarrier, const uint8_t* instdata, AmUnit* am, FmUnit* fm)
{
    mSlotIndex = iscarrier;
    mRawInstData = instdata;
    SetInstrument(0);
    
    mAm = am;
    mFm = fm;

    mFNum =     0;
    mBlock =    0;
    mFreqRate = 0;
    mPhase =    0;
    mRectify =  0;
    mFeedback = 0;

    mSustainBit =   0;
    mLevelAtten =   MaxAtten;
    mAdsr =         Idle;
    mEnvPhase =     0;
    
    mRate_Attack =      0;
    mRate_Decay =       0;
    mRate_Sustain =     0;
    mRate_Release =     0;
    mSustainLevel =     0;

    mOutput[0] = mOutput[1] = 0;
}

void Vrc7Audio::Slot::Write(uint8_t a, uint8_t v)
{
    switch(a & 0x30)
    {
    case 0x10:
        mFNum = (mFNum & 0x100) | v;
        UpdateFreqRate();
        break;

    case 0x20:
        mSustainBit =    v & 0x20;
        mBlock =        (v >> 1) & 7;
        mFNum =         (mFNum & 0x0FF) | ((v & 1) << 8);
        UpdateFreqRate();

        if(v & 0x10)        // Key on
        {
            if((mAdsr == Idle) || (mAdsr == Release))
                KeyOn();
        }
        else
        {
            if((mAdsr != Idle) && (mAdsr != Release))
                KeyOff();
        }
        break;

    case 0x30:
        SetInstrument(v >> 4);
        mVolume = (v & 0x0F) << 2;
        RefreshInst();
        break;
    }
}

inline void Vrc7Audio::Slot::SetInstrument(int inst)
{
    inst *= 8;
    for(int i = 0; i < 8; ++i)
        mInstData[i] = mRawInstData[i + inst];
}

inline int Vrc7Audio::Slot::Clock(uint32_t mod)
{
    mOutput[1] = mOutput[0];
    mOutput[0] = DoClock(mod);

    mFeedbackOutput = (mOutput[0] + mOutput[1]) / 2;
    //mFeedbackOutput = mOutput[0];
    return mFeedbackOutput;
}

int Vrc7Audio::Slot::DoClock(uint32_t mod)
{
    if(mAdsr == Idle)       // early out
        return 0;

    if(mFeedback)
        mod += mFeedbackOutput >> (8-mFeedback);

    // clock envelope
    uint32_t env = ClockEnvelope();

    // clock phase
    if(mInstData[mSlotIndex] & 0x40)
        mPhase += static_cast<uint32_t>( mFreqRate / 2 * mFm->Output() );
    else
        mPhase += mFreqRate / 2;

    mod += mPhase;          // output = mPhase + mod
    env += HalfSineLut[ (mod >> PhaseShift) & PhaseMask ];

    if(env >= MaxAtten)
        return 0;

    // convert dB Attenuation to linear output
    int output = LinearLut[ env >> LinearLutShift ];

    // rectify the sine wave
    if(mod & (1<<17))
        output *= mRectify;

    return output;
}

uint32_t Vrc7Audio::Slot::ClockEnvelope()
{
    uint32_t out;

    switch(mAdsr)
    {
    case Attack:
        out = AttackLut[ (mEnvPhase >> AttackLutShift) & AttackLutMask ];
        mEnvPhase += mRate_Attack;
        if(mEnvPhase >= MaxAtten)
        {
            mEnvPhase = 0;
            mAdsr = Decay;
        }
        break;

    case Decay:
        out = mEnvPhase;
        mEnvPhase += mRate_Decay;
        if(mEnvPhase >= mSustainLevel)
        {
            mEnvPhase = mSustainLevel;
            mAdsr = Sustain;
        }
        break;

    case Sustain:
        out = mEnvPhase;
        mEnvPhase += mRate_Sustain;
        if(mEnvPhase >= MaxAtten)
            mAdsr = Idle;
        break;

    case Release:
        out = mEnvPhase;
        mEnvPhase += mRate_Release;
        if(mEnvPhase >= MaxAtten)
            mAdsr = Idle;
        break;
    }

    if(mInstData[mSlotIndex & 0x80])
        out += mAm->Output();

    return out + mLevelAtten + mKslAtten;
}

inline void Vrc7Audio::Slot::RefreshInstZero()
{
    if(mRawInstData == mInstData)       // if we are using Inst Zero
        RefreshInst();                  //  refresh it
}

void Vrc7Audio::Slot::KeyOn()
{
    mPhase = 0;
    mEnvPhase = 0;
    mAdsr = Attack;
}

void Vrc7Audio::Slot::KeyOff()
{
    if(mAdsr == Attack)
        mEnvPhase = AttackLut[ (mEnvPhase >> AttackLutShift) & AttackLutMask ];
    mAdsr = Release;
}

void Vrc7Audio::Slot::UpdateFreqRate()
{
    static const int MultiLut[0x10] = {1,2,4,6,8,10,12,14,16,18,20,20,24,24,30,30};

    int multi = mInstData[0 | mSlotIndex] & 0x0F;

    mFreqRate = mFNum * (1 << mBlock) * MultiLut[ multi ] / 2;

    RecalcKeyScale();
}

void Vrc7Audio::Slot::RefreshInst()
{
    // Sine wave rectifying
    if( mInstData[3] & (0x8 << mSlotIndex) )        mRectify =  0;      // half-sine
    else                                            mRectify = -1;      // full-sine

    // Total level
    mLevelAtten = mSlotIndex ? mVolume : (mInstData[2] & 0x3F);     // bits of regs
    mLevelAtten = mLevelAtten * dB(0.75);                           // bits * 0.75 dB

    // Sustain level
    mSustainLevel = dB(3) * (mInstData[6 | mSlotIndex] >> 4);

    // Feedback level
    if(!mSlotIndex)     mFeedback = mInstData[3] & 0x07;
    else                mFeedback = 0;

    // The rest is KeyScale related
    RecalcKeyScale();
}

void Vrc7Audio::Slot::RecalcKeyScale()
{
    // Ksl
    int kslbits = mInstData[2 | mSlotIndex] >> 6;
    if(kslbits)
    {
        static const int KslTable[0x10] = 
        {
            dB(0),      dB(18),     dB(24),     dB(27.75),
            dB(30),     dB(32.25),  dB(33.75),  dB(35.25),
            dB(36),     dB(37.5),   dB(38.25),  dB(39),
            dB(39.75),  dB(40.5),   dB(41.25),  dB(42)
        };

        int a = KslTable[ mFNum >> 5 ] - dB(6)*(7 - mBlock);
        if(a <= 0)
            mKslAtten = 0;
        else
            mKslAtten = a >> (3-kslbits);
    }
    else
        mKslAtten = 0;

    //////////////////////////////////////////////////////
    // Ksr -- attack/decay/sustain/release rates
    struct
    {
        int K;              // 'K' value from Ksr
        int RL;             // Rate modifier Low
        int RH;             // Rate modifier High

        void R(int r)       // set 'R'
        {
            r = (r * 4) + K;
            RH = r>>2;
            if(RH > 15)
                RH = 15;
            RL = r&3;
        }
    } Ksr;

    Ksr.K = (mBlock << 1) | (mFNum >> 8);
    if(!(mInstData[mSlotIndex] & 0x10)) // if KSR is "off"
        Ksr.K >>= 2;

    // bits as written to reg
    mRate_Attack  = mInstData[4 | mSlotIndex] >> 4;
    mRate_Decay   = mInstData[4 | mSlotIndex] & 0x0F;
    mRate_Sustain = mInstData[6 | mSlotIndex] & 0x0F;
         if(mSustainBit)                            mRate_Release = 5;
    else if(mInstData[0 | mSlotIndex] & 0x20)       mRate_Release = mRate_Sustain;
    else                                            mRate_Release = 7;
    
    if(mInstData[0 | mSlotIndex] & 0x20)
        mRate_Sustain = 0;

    // convert
    if(mRate_Attack)
    {
        Ksr.R( mRate_Attack );
        mRate_Attack = (12 * (Ksr.RL+4)) << Ksr.RH;
    }
    if(mRate_Decay)
    {
        Ksr.R( mRate_Decay );
        mRate_Decay = (Ksr.RL+4) << (Ksr.RH-1);
    }
    if(mRate_Sustain)
    {
        Ksr.R( mRate_Sustain );
        mRate_Sustain = (Ksr.RL+4) << (Ksr.RH-1);
    }
    if(mRate_Release)
    {
        Ksr.R( mRate_Release );
        mRate_Release = (Ksr.RL+4) << (Ksr.RH-1);
    }
}

/////////////////////////////////////////////////////////////////
//  Channel

inline void Vrc7Audio::Channel::Reset(bool hard, const uint8_t* instdata, AmUnit* am, FmUnit* fm)
{
    mMod.Reset(hard,0,instdata,am,fm);
    mCar.Reset(hard,1,instdata,am,fm);
}

inline void Vrc7Audio::Channel::Write(int a,uint8_t v)
{
#ifndef NO_MODULATOR
    mMod.Write(a,v);
#endif
    mCar.Write(a,v);
}

inline void Vrc7Audio::Channel::RefreshInstZero()
{
    mMod.RefreshInstZero();
    mCar.RefreshInstZero();
}

inline int Vrc7Audio::Channel::Clock()
{
#ifndef NO_MODULATOR
    return mCar.Clock( mMod.Clock(0) );
#else
    return mCar.Clock( 0 );
#endif
}

}// schpune
