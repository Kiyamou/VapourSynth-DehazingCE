

#include "DehazingCE.h"

/*
    Function: CalcAcoeff (called after Boxfilter)
    Description: calculate the coefficent "a" of guided filter (intrinsic function for guide filtering)
    Parameters:
        pfSigma - Sigma + eps * eye(3) --> see the paper and original matlab code
        pfCov - Cov of image
        nIdx - location of image(index).
    Return:
        pfA1, pfA2, pfA3 - coefficient of "a" at each color channel
 */
void dehazing::CalcAcoeff(float* pfSigma, float* pfCov, float* pfA1, float* pfA2, float* pfA3, int nIdx)
{
    float fDet;
    float fOneOverDeterminant;
    float pfInvSig[9];

    int nIdx9 = nIdx * 9;

    // a_k = (sum_i(I_i*p_i-mu_k*p_k)/(abs(omega)*(sigma_k^2+epsilon))
    fDet = ((pfSigma[nIdx9] * (pfSigma[nIdx9 + 4] * pfSigma[nIdx9 + 8] - pfSigma[nIdx9 + 5] * pfSigma[nIdx9 + 7]))
        - (pfSigma[nIdx9 + 1] * (pfSigma[nIdx9 + 3] * pfSigma[nIdx9 + 8] - pfSigma[nIdx9 + 5] * pfSigma[nIdx9 + 6]))
        + (pfSigma[nIdx9 + 2] * (pfSigma[nIdx9 + 3] * pfSigma[nIdx9 + 7] - pfSigma[nIdx9 + 4] * pfSigma[nIdx9 + 6])));
    fOneOverDeterminant = 1.f / fDet;

    pfInvSig[0] = ((pfSigma[nIdx9 + 4] * pfSigma[nIdx9 + 8]) - (pfSigma[nIdx9 + 5] * pfSigma[nIdx9 + 7])) * fOneOverDeterminant;
    pfInvSig[1] = -((pfSigma[nIdx9 + 1] * pfSigma[nIdx9 + 8]) - (pfSigma[nIdx9 + 2] * pfSigma[nIdx9 + 7])) * fOneOverDeterminant;
    pfInvSig[2] = ((pfSigma[nIdx9 + 1] * pfSigma[nIdx9 + 5]) - (pfSigma[nIdx9 + 2] * pfSigma[nIdx9 + 4])) * fOneOverDeterminant;
    pfInvSig[3] = -((pfSigma[nIdx9 + 3] * pfSigma[nIdx9 + 8]) - (pfSigma[nIdx9 + 5] * pfSigma[nIdx9 + 6])) * fOneOverDeterminant;
    pfInvSig[4] = ((pfSigma[nIdx9] * pfSigma[nIdx9 + 8]) - (pfSigma[nIdx9 + 2] * pfSigma[nIdx9 + 6])) * fOneOverDeterminant;
    pfInvSig[5] = -((pfSigma[nIdx9] * pfSigma[nIdx9 + 5]) - (pfSigma[nIdx9 + 2] * pfSigma[nIdx9 + 3])) * fOneOverDeterminant;
    pfInvSig[6] = ((pfSigma[nIdx9 + 3] * pfSigma[nIdx9 + 7]) - (pfSigma[nIdx9 + 4] * pfSigma[nIdx9 + 6])) * fOneOverDeterminant;
    pfInvSig[7] = -((pfSigma[nIdx9] * pfSigma[nIdx9 + 7]) - (pfSigma[nIdx9 + 1] * pfSigma[nIdx9 + 6])) * fOneOverDeterminant;
    pfInvSig[8] = ((pfSigma[nIdx9] * pfSigma[nIdx9 + 4]) - (pfSigma[nIdx9 + 1] * pfSigma[nIdx9 + 3])) * fOneOverDeterminant;

    int nIdx3 = nIdx * 3;

    pfA1[nIdx] = pfCov[nIdx3] * pfInvSig[0] + pfCov[nIdx3 + 1] * pfInvSig[3] + pfCov[nIdx3 + 2] * pfInvSig[6];
    pfA2[nIdx] = pfCov[nIdx3] * pfInvSig[1] + pfCov[nIdx3 + 1] * pfInvSig[4] + pfCov[nIdx3 + 2] * pfInvSig[7];
    pfA3[nIdx] = pfCov[nIdx3] * pfInvSig[2] + pfCov[nIdx3 + 1] * pfInvSig[5] + pfCov[nIdx3 + 2] * pfInvSig[8];
}


/*
    Function: BoxFilter
    Description: cummulative function for calculating the integral image (It may apply other arraies.)
    Parameters:
        pfInArray - input array
        nR - radius of filter window
        width - width of array
        height - height of array
    Return:
        fOutArray - output array (integrated array)
 */
void dehazing::BoxFilter(float* pfInArray, int nR, int width, int height, float*& fOutArray)
{
    float* pfArrayCum = new float[width * height];

    //cumulative sum over Y axis
    for (auto i = 0; i < width; i++)
        pfArrayCum[i] = pfInArray[i];

    for (int nIdx = width; nIdx < width * height; nIdx++)
        pfArrayCum[nIdx] = pfArrayCum[nIdx - width] + pfInArray[nIdx];

    //difference over Y axis
    for (int nIdx = 0; nIdx < width * (nR + 1); nIdx++)
        fOutArray[nIdx] = pfArrayCum[nIdx + nR * width];

    for (int nIdx = (nR + 1) * width; nIdx < (height - nR) * width; nIdx++)
        fOutArray[nIdx] = pfArrayCum[nIdx + nR * width] - pfArrayCum[nIdx - nR * width - width];

    for (auto j = height - nR; j < height; j++)
        for (auto i = 0; i < width; i++)
            fOutArray[j * width + i] = pfArrayCum[(height - 1) * width + i] - pfArrayCum[(j - nR - 1) * width + i];

    //cumulative sum over X axis
    for (int nIdx = 0; nIdx < width * height; nIdx += width)
        pfArrayCum[nIdx] = fOutArray[nIdx];

    for (auto j = 0; j < width * height; j += width)
        for (auto i = 1; i < width; i++)
            pfArrayCum[j + i] = pfArrayCum[j + i - 1] + fOutArray[j + i];

    //difference over Y axis
    for (auto j = 0; j < width * height; j += width)
        for (auto i = 0; i < nR + 1; i++)
            fOutArray[j + i] = pfArrayCum[j + i + nR];

    for (auto j = 0; j < width * height; j += width)
        for (auto i = nR + 1; i < width - nR; i++)
            fOutArray[j + i] = pfArrayCum[j + i + nR] - pfArrayCum[j + i - nR - 1];

    for (auto j = 0; j < width * height; j += width)
        for (auto i = width - nR; i < width; i++)
            fOutArray[j + i] = pfArrayCum[j + width - 1] - pfArrayCum[j + i - nR - 1];

    delete[] pfArrayCum;
}

/*
    Function: BoxFilter (for 3D array)
    Description: cummulative function for calculating the integral image (It may apply other arraies.)
    Parameters:
        pfInArray1 - input array D1
        pfInArray2 - input array D2
        pfInArray3 - input array D3
        nR - radius of filter window
        width - width of array
        height - height of array
    Return:
        fOutArray1 - output array D1(integrated array)
        fOutArray1 - output array D2(integrated array)
        fOutArray1 - output array D3(integrated array)
 */
void dehazing::BoxFilter(float* pfInArray1, float* pfInArray2, float* pfInArray3, int nR, int width, int height, float*& pfOutArray1, float*& pfOutArray2, float*& pfOutArray3)
{
    float* pfArrayCum1 = new float[width * height];
    float* pfArrayCum2 = new float[width * height];
    float* pfArrayCum3 = new float[width * height];

    //cumulative sum over Y axis
    for (auto i = 0; i < width; i++)
    {
        pfArrayCum1[i] = pfInArray1[i];
        pfArrayCum2[i] = pfInArray2[i];
        pfArrayCum3[i] = pfInArray3[i];
    }

    for (auto nIdx = width; nIdx < width * height; nIdx++)
    {
        pfArrayCum1[nIdx] = pfArrayCum1[nIdx - width] + pfInArray1[nIdx];
        pfArrayCum2[nIdx] = pfArrayCum2[nIdx - width] + pfInArray2[nIdx];
        pfArrayCum3[nIdx] = pfArrayCum3[nIdx - width] + pfInArray3[nIdx];
    }

    //difference over Y axis
    for (auto nIdx = 0; nIdx < (nR + 1) * width; nIdx++)
    {
        pfOutArray1[nIdx] = pfArrayCum1[nIdx + nR * width];
        pfOutArray2[nIdx] = pfArrayCum2[nIdx + nR * width];
        pfOutArray3[nIdx] = pfArrayCum3[nIdx + nR * width];
    }

    for (auto nIdx = (nR + 1) * width; nIdx < (height - nR) * width; nIdx++)
    {
        pfOutArray1[nIdx] = pfArrayCum1[nIdx + nR * width] - pfArrayCum1[nIdx - (nR + 1) * width];
        pfOutArray2[nIdx] = pfArrayCum2[nIdx + nR * width] - pfArrayCum2[nIdx - (nR + 1) * width];
        pfOutArray3[nIdx] = pfArrayCum3[nIdx + nR * width] - pfArrayCum3[nIdx - (nR + 1) * width];
    }

    for (auto j = height - nR; j < height; j++)
    {
        for (auto i = 0; i < width; i++)
        {
            pfOutArray1[j * width + i] = pfArrayCum1[(height - 1) * width + i] - pfArrayCum1[(j - nR - 1) * width + i];
            pfOutArray2[j * width + i] = pfArrayCum2[(height - 1) * width + i] - pfArrayCum2[(j - nR - 1) * width + i];
            pfOutArray3[j * width + i] = pfArrayCum3[(height - 1) * width + i] - pfArrayCum3[(j - nR - 1) * width + i];
        }
    }

    //cumulative sum over X axis
    for (auto j = 0; j < width * height; j += width)
    {
        pfArrayCum1[j] = pfOutArray1[j];
        pfArrayCum2[j] = pfOutArray2[j];
        pfArrayCum3[j] = pfOutArray3[j];
    }

    for (auto j = 0; j < width * height; j += width)
    {
        for (auto i = 0; i < width; i++)
        {
            pfArrayCum1[j + i] = pfArrayCum1[j + i - 1] + pfOutArray1[j + i];
            pfArrayCum2[j + i] = pfArrayCum2[j + i - 1] + pfOutArray2[j + i];
            pfArrayCum3[j + i] = pfArrayCum3[j + i - 1] + pfOutArray3[j + i];
        }
    }

    //difference over Y axis
    for (auto j = 0; j < width * height; j += width)
    {
        for (auto i = 0; i < nR + 1; i++)
        {
            pfOutArray1[j + i] = pfArrayCum1[j + i + nR];
            pfOutArray2[j + i] = pfArrayCum2[j + i + nR];
            pfOutArray3[j + i] = pfArrayCum3[j + i + nR];
        }
    }

    for (auto j = 0; j < width * height; j += width)
    {
        for (auto i = nR + 1; i < width - nR; i++)
        {
            pfOutArray1[j + i] = pfArrayCum1[j + i + nR] - pfArrayCum1[j + i - nR - 1];
            pfOutArray2[j + i] = pfArrayCum2[j + i + nR] - pfArrayCum2[j + i - nR - 1];
            pfOutArray3[j + i] = pfArrayCum3[j + i + nR] - pfArrayCum3[j + i - nR - 1];
        }
    }

    for (auto j = 0; j < width * height; j += width)
    {
        for (auto i = width - nR; i < width; i++)
        {
            pfOutArray1[j + i] = pfArrayCum1[j + width - 1] - pfArrayCum1[j + i - nR - 1];
            pfOutArray2[j + i] = pfArrayCum2[j + width - 1] - pfArrayCum2[j + i - nR - 1];
            pfOutArray3[j + i] = pfArrayCum3[j + width - 1] - pfArrayCum3[j + i - nR - 1];
        }
    }

    delete[] pfArrayCum1;
    delete[] pfArrayCum2;
    delete[] pfArrayCum3;
}



/*
	Function: GuidedFilter
	Description: the original guided filter for rgb color image. This function is used for image dehazing.
		The video dehazing algorithm uses appoximated filter for fast refinement.
	Parameter:
		nW - width of array
		nH - height of array
		fEps - epsilon
	(member variable)
		m_pfTransmission - initial transmission (block_based)
		m_pnYImg - guidance image (Y image)
	Return:
		m_pfTransmissionR - filtered transmission 为计算m_pfTransmissionR，话说这个R是什么意思
 */
void dehazing::GuidedFilter(int width, int height, float fEps)
{
    float* pfImageR = new float[width * height];
    float* pfImageG = new float[width * height];
    float* pfImageB = new float[width * height];

    float* pfInitN = new float[width * height];
    float* pfInitMeanIpR = new float[width * height];
    float* pfInitMeanIpG = new float[width * height];
    float* pfInitMeanIpB = new float[width * height];
    float* pfMeanP = new float[width * height];

    float* pfN = new float[width * height];
    float* pfMeanIr = new float[width * height];
    float* pfMeanIg = new float[width * height];
    float* pfMeanIb = new float[width * height];
    float* pfMeanIpR = new float[width * height];
    float* pfMeanIpG = new float[width * height];
    float* pfMeanIpB = new float[width * height];
    float* pfCovIpR = new float[width * height];
    float* pfCovIpG = new float[width * height];
    float* pfCovIpB = new float[width * height];

    float* pfCovEntire = new float[width * height * 3];

    float* pfInitVarIrr = new float[width * height];
    float* pfInitVarIrg = new float[width * height];
    float* pfInitVarIrb = new float[width * height];
    float* pfInitVarIgg = new float[width * height];
    float* pfInitVarIgb = new float[width * height];
    float* pfInitVarIbb = new float[width * height];

    float* pfVarIrr = new float[width * height];
    float* pfVarIrg = new float[width * height];
    float* pfVarIrb = new float[width * height];
    float* pfVarIgg = new float[width * height];
    float* pfVarIgb = new float[width * height];
    float* pfVarIbb = new float[width * height];

    float* pfA1 = new float[width * height];
    float* pfA2 = new float[width * height];
    float* pfA3 = new float[width * height];
    float* pfOutA1 = new float[width * height];
    float* pfOutA2 = new float[width * height];
    float* pfOutA3 = new float[width * height];

    float* pfSigmaEntire = new float[width * height * 9];

    float* pfB = new float[width * height];
    float* pfOutB = new float[width * height];

    // Converting to float point
    for (auto nIdx = 0; nIdx < width * height; nIdx++)
    {
        pfImageR[nIdx] = (float)m_pnRImg[nIdx];
        pfImageG[nIdx] = (float)m_pnGImg[nIdx];
        pfImageB[nIdx] = (float)m_pnBImg[nIdx];
    }
    //////////////////////////////////////////////////////////////////////////

    // Make an integral image
    for (auto nIdx = 0; nIdx < width * height; nIdx++)
    {
        pfInitN[nIdx] = 1.f;

        pfInitMeanIpR[nIdx] = pfImageR[nIdx] * m_pfTransmission[nIdx];
        pfInitMeanIpG[nIdx] = pfImageG[nIdx] * m_pfTransmission[nIdx];
        pfInitMeanIpB[nIdx] = pfImageB[nIdx] * m_pfTransmission[nIdx];
    }

    BoxFilter(pfInitN, GBlockSize, width, height, pfN);
    BoxFilter(m_pfTransmission, GBlockSize, width, height, pfMeanP);

    BoxFilter(pfImageR, pfImageG, pfImageB, GBlockSize, width, height, pfMeanIr, pfMeanIg, pfMeanIb);

    BoxFilter(pfInitMeanIpR, pfInitMeanIpG, pfInitMeanIpB, GBlockSize, width, height, pfMeanIpR, pfMeanIpG, pfMeanIpB);

    //Covariance of (I, pfTrans) in each local patch

    for (auto nIdx = 0; nIdx < width * height; nIdx++)
    {
        pfMeanIr[nIdx] = pfMeanIr[nIdx] / pfN[nIdx];
        pfMeanIg[nIdx] = pfMeanIg[nIdx] / pfN[nIdx];
        pfMeanIb[nIdx] = pfMeanIb[nIdx] / pfN[nIdx];

        pfMeanP[nIdx] = pfMeanP[nIdx] / pfN[nIdx];

        pfMeanIpR[nIdx] = pfMeanIpR[nIdx] / pfN[nIdx];
        pfMeanIpG[nIdx] = pfMeanIpG[nIdx] / pfN[nIdx];
        pfMeanIpB[nIdx] = pfMeanIpB[nIdx] / pfN[nIdx];

        pfCovIpR[nIdx] = pfMeanIpR[nIdx] - pfMeanIr[nIdx] * pfMeanP[nIdx];
        pfCovIpG[nIdx] = pfMeanIpG[nIdx] - pfMeanIg[nIdx] * pfMeanP[nIdx];
        pfCovIpB[nIdx] = pfMeanIpB[nIdx] - pfMeanIb[nIdx] * pfMeanP[nIdx];

        pfCovEntire[nIdx * 3] = pfCovIpR[nIdx];
        pfCovEntire[nIdx * 3 + 1] = pfCovIpG[nIdx];
        pfCovEntire[nIdx * 3 + 2] = pfCovIpB[nIdx];

        pfInitVarIrr[nIdx] = pfImageR[nIdx] * pfImageR[nIdx];
        pfInitVarIrg[nIdx] = pfImageR[nIdx] * pfImageG[nIdx];
        pfInitVarIrb[nIdx] = pfImageR[nIdx] * pfImageB[nIdx];
        pfInitVarIgg[nIdx] = pfImageG[nIdx] * pfImageG[nIdx];
        pfInitVarIgb[nIdx] = pfImageG[nIdx] * pfImageB[nIdx];
        pfInitVarIbb[nIdx] = pfImageB[nIdx] * pfImageB[nIdx];
    }

    // Variance of I in each local patch: the matrix Sigma.
    // 		    rr, rg, rb
    // pfSigma  rg, gg, gb
    //	 	    rb, gb, bb

    BoxFilter(pfInitVarIrr, pfInitVarIrg, pfInitVarIrb, GBlockSize, width, height, pfVarIrr, pfVarIrg, pfVarIrb);
    BoxFilter(pfInitVarIgg, pfInitVarIgb, pfInitVarIbb, GBlockSize, width, height, pfVarIgg, pfVarIgb, pfVarIbb);

    for (auto nIdx = 0; nIdx < width * height; nIdx++)
    {
        pfVarIrr[nIdx] = pfVarIrr[nIdx] / pfN[nIdx] - pfMeanIr[nIdx] * pfMeanIr[nIdx];
        pfVarIrg[nIdx] = pfVarIrg[nIdx] / pfN[nIdx] - pfMeanIr[nIdx] * pfMeanIg[nIdx];
        pfVarIrb[nIdx] = pfVarIrb[nIdx] / pfN[nIdx] - pfMeanIr[nIdx] * pfMeanIb[nIdx];
        pfVarIgg[nIdx] = pfVarIgg[nIdx] / pfN[nIdx] - pfMeanIg[nIdx] * pfMeanIg[nIdx];
        pfVarIgb[nIdx] = pfVarIgb[nIdx] / pfN[nIdx] - pfMeanIg[nIdx] * pfMeanIb[nIdx];
        pfVarIbb[nIdx] = pfVarIbb[nIdx] / pfN[nIdx] - pfMeanIb[nIdx] * pfMeanIb[nIdx];

        pfSigmaEntire[nIdx * 9 + 0] = pfVarIrr[nIdx] + fEps * 2.f;
        pfSigmaEntire[nIdx * 9 + 1] = pfVarIrg[nIdx];
        pfSigmaEntire[nIdx * 9 + 2] = pfVarIrb[nIdx];
        pfSigmaEntire[nIdx * 9 + 3] = pfVarIrg[nIdx];
        pfSigmaEntire[nIdx * 9 + 4] = pfVarIgg[nIdx] + fEps * 2.f;
        pfSigmaEntire[nIdx * 9 + 5] = pfVarIgb[nIdx];
        pfSigmaEntire[nIdx * 9 + 6] = pfVarIrb[nIdx];
        pfSigmaEntire[nIdx * 9 + 7] = pfVarIgb[nIdx];
        pfSigmaEntire[nIdx * 9 + 8] = pfVarIbb[nIdx] + fEps * 2.f;
    }
    // Calculate coefficient a and coefficient b
    // Coefficienta
    for (auto nIdx = 0; nIdx < width * height; nIdx++)
    {
        CalcAcoeff(pfSigmaEntire, pfCovEntire, pfA1, pfA2, pfA3, nIdx);
    }

    // Coefficient b
    for (auto nIdx = 0; nIdx < width * height; nIdx++)
    {
        pfB[nIdx] = pfMeanP[nIdx] - pfA1[nIdx] * pfMeanIr[nIdx] - pfA2[nIdx] * pfMeanIg[nIdx] - pfA3[nIdx] * pfMeanIb[nIdx];
    }

    // Transmission refinement at each pixel
    BoxFilter(pfA1, pfA2, pfA3, GBlockSize, width, height, pfOutA1, pfOutA2, pfOutA3);

    BoxFilter(pfB, GBlockSize, width, height, pfOutB);

    for (auto nIdx = 0; nIdx < width * height; nIdx++)
    {
        m_pfTransmissionR[nIdx] = (pfOutA1[nIdx] * pfImageR[nIdx] + pfOutA2[nIdx] * pfImageG[nIdx] + pfOutA3[nIdx] * pfImageB[nIdx] + pfOutB[nIdx]) / pfN[nIdx];
    }

    delete[] pfInitN;
    delete[] pfInitMeanIpR;
    delete[] pfInitMeanIpG;
    delete[] pfInitMeanIpB;
    delete[] pfMeanP;

    delete[] pfN;
    delete[] pfMeanIr;
    delete[] pfMeanIg;
    delete[] pfMeanIb;
    delete[] pfMeanIpR;
    delete[] pfMeanIpG;
    delete[] pfMeanIpB;
    delete[] pfCovIpR;
    delete[] pfCovIpG;
    delete[] pfCovIpB;
    delete[] pfCovEntire;
    delete[] pfInitVarIrr;
    delete[] pfInitVarIrg;
    delete[] pfInitVarIrb;
    delete[] pfInitVarIgg;
    delete[] pfInitVarIgb;
    delete[] pfInitVarIbb;
    delete[] pfVarIrr;
    delete[] pfVarIrg;
    delete[] pfVarIrb;
    delete[] pfVarIgg;
    delete[] pfVarIgb;
    delete[] pfVarIbb;
    delete[] pfA1;
    delete[] pfA2;
    delete[] pfA3;
    delete[] pfOutA1;
    delete[] pfOutA2;
    delete[] pfOutA3;
    delete[] pfSigmaEntire;
    delete[] pfB;
    delete[] pfOutB;

    delete[] pfImageR;
    delete[] pfImageG;
    delete[] pfImageB;
}
