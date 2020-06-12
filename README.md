# VapourSynth-DehazingCE
DehazingCE is a dehazing plugin for VapourSynth, based on contrast enhancement.

Original paper: [Optimized contrast enhancement for real-time image and video dehazing](http://mcl.korea.ac.kr/projects/dehazing/#userconsent#)

Still in development, only support 8bit RGB now.

## Usage

```python
core.dhce.Dehazing(clip clip[, clip ref, int guide_size, int trans_size, bool post])
```

* ***clip***
    * Required parameter.
    * Clip to process.
    * Only support 8bit RGB now.
* ***ref***
    * Optional parameter. *Default: same with input*.
* ***guide_size***
    * Optional parameter. *Default: 40*.
    * Block size in guide filter.
* ***trans_size***
    * Optional parameter. *Default: 16*.
    * Block size in transmission estimation.
* ***post***
    * Optional parameter. *Default: False*.
    * Whether to post-process.

## License

[License](https://github.com/Kiyamou/VapourSynth-DehazingCE/blob/master/LICENSE) is from original source code.