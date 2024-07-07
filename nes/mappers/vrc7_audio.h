
#ifndef SCHPUNE_VRC7AUDIO_H_INCLUDED
#define SCHPUNE_VRC7AUDIO_H_INCLUDED
#include <stdint.h>
namespace schpune
{

class AudioBuilder;

class Vrc7Audio
{
public:
    void            Reset(bool hard);
    void            Write( uint16_t a, uint8_t v );
    int            clock( int ticks );
    int audio_out = 0;

private:
    uint8_t         mAddr;
    uint8_t         mInstData[0x80];

    enum Adsr
    {
        Idle        = 0,
        Attack,
        Decay,
        Sustain,
        Release
    };

    class AmUnit
    {
    public:
        void            Reset(bool hard);
        void            Clock();
        uint32_t        Output()        { return mOut;  }

    private:
        uint32_t        mPhase;
        uint32_t        mOut;
    };
    
    class FmUnit
    {
    public:
        void            Reset(bool hard);
        void            Clock();
        double          Output()        { return mOut;  }

    private:
        uint32_t        mPhase;
        double          mOut;
    };

    class Slot
    {
    public:
        void            Reset(bool hard,int iscarrier, const uint8_t* instdata, AmUnit* am, FmUnit* fm);
        void            Write(uint8_t a, uint8_t v);
        int             Clock(uint32_t mod);
        void            RefreshInstZero();

    private:
        // helper function
        void            RefreshInst();
        void            UpdateFreqRate();
        void            RecalcKeyScale();
        uint32_t        ClockEnvelope();
        int             DoClock(uint32_t mod);
        void            KeyOn();
        void            KeyOff();
        void            SetInstrument(int inst);

        // slot type
        int             mSlotIndex;     // 0=Modulator, 1=Carrier

        // instrument data
        const uint8_t*  mRawInstData;
        uint8_t         mInstData[8];

        // am/fm units
        AmUnit*         mAm;
        FmUnit*         mFm;

        // phase generator
        int             mFNum;          // 9-bit F number
        int             mBlock;         // 3-bit block (octave)
        int             mFreqRate;      // value to add to phase counter (based on FNum,block,Multi)
        uint32_t        mPhase;         // phase counter
        int             mRectify;       // 0 if half-sine, -1 if full sine
        int             mFeedback;      // Feedback value as written to reg (0 = no feedback)

        // envelope generator
        int             mSustainBit;    // nonzero if sustain bit in channel is set (reg $2x.5)
        uint32_t        mLevelAtten;    // Base attenuation level (TL or volume)
        uint32_t        mKslAtten;      // attenuation level (KSL)
        int             mVolume;        // Volume (bits as written to $3x, << 2) - only used on Carrier
        Adsr            mAdsr;          // ADSR mode
        uint32_t        mEnvPhase;      // envelope phase counter  (rate = adsr rate)

        uint32_t        mRate_Attack;   // Attack rate
        uint32_t        mRate_Decay;    // Decay rate
        uint32_t        mRate_Sustain;  // Sustain rate
        uint32_t        mRate_Release;  // Release rate
        uint32_t        mSustainLevel;  // Sustain Level

        // previous output
        int             mFeedbackOutput;
        int             mOutput[2];
    };

    class Channel
    {
    public:
        void            Reset(bool hard, const uint8_t* instdata, AmUnit* am, FmUnit* fm);
        void            Write(int a,uint8_t v);
        void            RefreshInstZero();
        int             Clock();

    private:
        Slot            mCar;
        Slot            mMod;
    };
    
    AmUnit          mAmUnit;
    FmUnit          mFmUnit;
    int             mTicksToNextClock;
    int             mPrevOut;
    Channel         mChannel[6];
};

} // schpune

#endif //  SCHPUNE_VRC7AUDIO_H_INCLUDED