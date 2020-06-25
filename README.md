# VapourSynth-DehazingCE

[![Build Status](https://api.travis-ci.org/Kiyamou/VapourSynth-DehazingCE.svg?branch=master)](https://api.travis-ci.org/Kiyamou/VapourSynth-DehazingCE.svg?branch=master)

DehazingCE is a dehazing plugin for VapourSynth, based on contrast enhancement.

Original paper: [Optimized contrast enhancement for real-time image and video dehazing](http://mcl.korea.ac.kr/projects/dehazing/#userconsent#)

Still in development, support 8-16 bit RGB.

## Usage

```python
core.dhce.Dehazing(clip src[, clip ref, float trans, int trans_size, int guide_size, float gamma, bool post])
```

* ***src***
    * Required parameter.
    * Clip to process.
    * Support 8-16 bit RGB.
* ***ref***
    * Optional parameter. *Default: src*.
* ***trans***
    * Optional parameter. *Default: 0.3*.
    * Initial value of transmission.
    * The larger the initial value, the stronger the effect of dehazing, but the contrast may be too high.
* ***trans_size***
    * Optional parameter. *Default: 16*.
    * Block size in transmission estimation.
* ***guide_size***
    * Optional parameter. *Default: 40*.
    * Block size in guide filter.
* ***gamma***
    * Optional parameter. *Default: 0.7*.
    * Increase brightness to avoid image darkening after dehazing.
* ***post***
    * Optional parameter. *Default: False*.
    * Whether to post-process.

## Example

| Before dehazing | After dehazing |
| :-------------: | :------------: |
| <img width="360" src="https://i.loli.net/2020/06/13/h5jZJoc4KtSeuRn.jpg"> | <img width="360" src="https://i.loli.net/2020/06/13/oxBebyIunc97gOQ.jpg"> |
| <img width="360" src="https://i.loli.net/2020/06/12/rnjvJQdM6a3BZIg.jpg"> | <img width="360" src="https://i.loli.net/2020/06/12/hqgX9veIykwiL1r.jpg"> |

*The first image is from original paper. (Dehazing parameters: trans=0.4, gamma=0.6)*
*The second image is from [Wikipedia](https://en.wikipedia.org/wiki/File:20080313_Foggy_Street.jpg). (Dehazing parameters: trans=0.3, gamma=0.7)*

## License

[License](https://github.com/Kiyamou/VapourSynth-DehazingCE/blob/master/LICENSE) is from original source code.
