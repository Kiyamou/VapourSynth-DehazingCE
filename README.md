# VapourSynth-DehazingCE
DehazingCE is a dehazing plugin for VapourSynth, based on contrast enhancement.

Original paper: [Optimized contrast enhancement for real-time image and video dehazing](http://mcl.korea.ac.kr/projects/dehazing/#userconsent#)

Still in development, only support 8bit RGB now.

## Usage

```python
core.dhce.Dehazing(clip src[, clip ref, int guide_size, int trans_size, float gamma, bool post])
```

* ***src***
    * Required parameter.
    * Clip to process.
    * Only support 8bit RGB now.
* ***ref***
    * Optional parameter. *Default: src*.
* ***guide_size***
    * Optional parameter. *Default: 40*.
    * Block size in guide filter.
* ***trans_size***
    * Optional parameter. *Default: 16*.
    * Block size in transmission estimation.
* ***gamma***
    * Optional parameter. *Default: 0.7*.
    * Increase brightness to avoid image darkening after dehazing.
* ***post***
    * Optional parameter. *Default: False*.
    * Whether to post-process.

## Example

| Before dehazing | After dehazing |
| :-------------: | :------------: |
| ![](https://i.loli.net/2020/06/12/rnjvJQdM6a3BZIg.jpg) | ![](https://i.loli.net/2020/06/12/hqgX9veIykwiL1r.jpg) |

*The original image is from https://en.wikipedia.org/wiki/File:20080313_Foggy_Street.jpg.*

## License

[License](https://github.com/Kiyamou/VapourSynth-DehazingCE/blob/master/LICENSE) is from original source code.
