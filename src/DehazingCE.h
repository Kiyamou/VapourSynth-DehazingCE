

#ifndef DEHAZINGCE_H_
#define DEHAZINGCE_H_

#include "vapoursynth/VapourSynth.h"
#include "vapoursynth/VSHelper.h"


class dehazing
{
public:
    dehazing(int nW, int nH, int nTBlockSize, bool bPrevFlag, bool bPosFlag, float fL1, float fL2, int nGBlockSize);
    ~dehazing();

    void RemoveHaze(const uint8_t* src, const uint8_t* refpB, const uint8_t* refpG, const uint8_t* refpR, uint8_t* dst, int stride, int ref_width, int ref_height);

    void AirlightEstimation(const uint8_t* src, int width, int height, int stride);

	float NFTrsEstimationColor(const uint8_t* pnImageR, const uint8_t* pnImageG, const uint8_t* pnImageB, int nStartX, int nStartY, int ref_width, int ref_height);
    void TransmissionEstimationColor(const uint8_t* pnImageR, const uint8_t* pnImageG, const uint8_t* pnImageB, int ref_width, int ref_height);

    void PostProcessing(const uint8_t* src, uint8_t* dst, int width, int height, int stride);  // Called by RestoreImage();
    void RestoreImage(const uint8_t* src, uint8_t* dst, int height, int width, int stride);

    void CalcAcoeff(float* pfSigma, float* pfCov, float* pfA1, float* pfA2, float* pfA3, int nIdx);
    void BoxFilter(float* pfInArray, int nR, int nWid, int nHei, float*& fOutArray);
    void BoxFilter(float* pfInArray1, float* pfInArray2, float* pfInArray3, int nR, int nWid, int nHei, float*& pfOutArray1, float*& pfOutArray2, float*& pfOutArray3);

    void GuidedFilter(int nW, int nH, float fEps);

private:
    int* pnImageY;
    int* pnImageYP;

    int width;
    int height;

    int TBlockSize;
    int GBlockSize;
    int StepSize;
    float GSigma;

    bool m_PreviousFlag;
    bool m_PostFlag;  // Flag for post processing(deblocking)

    float Lambda1;
    float Lambda2;

    int TopLeftX;
    int TopLeftY;
    int BottomRightX;
    int BottomRightY;

    //////////////////////////////////
    //320*240 size
    float* m_pfSmallY;       // Y image
    float* m_pfSmallPk_p;    // (Y image) - (mean of Y image)
    float* m_pfSmallNormPk;
    float* m_pfSmallInteg;
    float* m_pfSmallDenom;

    int* m_pnSmallYImg;

    int* m_pnSmallRImg;
    int* m_pnSmallGImg;
    int* m_pnSmallBImg;

    float* m_pfSmallTransP;
    float* m_pfSmallTrans;
    float* m_pfSmallTransR;
    int* m_pnSmallYImgP;

    int* m_pnSmallRImgP;
    int* m_pnSmallGImgP;
    int* m_pnSmallBImgP;

    // Original size
    float* m_pfY;      // Y image
    float* m_pfPk_p;   // (Y image) - (mean of Y image)
    float* m_pfNormPk;
    float* m_pfInteg;
    float* m_pfDenom;

    int* m_pnYImg;
    int* m_pnYImgP;

    int* m_pnRImg;
    int* m_pnGImg;
    int* m_pnBImg;

    int* m_pnRImgP;
    int* m_pnGImgP;
    int* m_pnBImgP;

    float* m_pfTransmission;   // preliminary transmission
    float* m_pfTransmissionP;
    float* m_pfTransmissionR;  // refined transmission
    //////////////////////////////

public:
    void MakeExpLUT();
    void GuideLUTMaker();
    void GammaLUTMaker(float fParameter);

    float ExpLUT[256];
    float m_pucGammaLUT[256];
    float* m_pfGuidedLUT;

private:
    float fParameter;
    int m_anAirlight[3];
    int m_nAirlight;

    // Airlight search range
    int m_nTopLeftX;
    int m_nTopLeftY;
    int m_nBottomRightX;
    int m_nBottomRightY;
};


#endif
