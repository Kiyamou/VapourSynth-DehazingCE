# VapourSynth-DehazingCE

[![Build Status](https://github.com/Kiyamou/VapourSynth-DehazingCE/workflows/CI/badge.svg)](https://github.com/Kiyamou/VapourSynth-DehazingCE/actions)

DehazingCE is a dehazing plugin for VapourSynth, based on contrast enhancement.

Original paper: [Optimized contrast enhancement for real-time image and video dehazing](http://mcl.korea.ac.kr/projects/dehazing/#userconsent#)

Still in development, support 8-16 bit RGB.

## Usage

```python
core.dhce.Dehazing(clip src[, clip ref, float trans, float gamma, int air_size, int trans_size, int guide_size, bool post, float lamda])
```

* ***src***
    * Required parameter.
    * Clip to process.
    * Support 8-16 bit RGB.
* ***ref***
    * Optional parameter. *Default: src*.
    * According to the original code of the algorithm author and my test, **the size of ref clip recommends to set as 320 * 240**, which can avoid uneven lighting to a certain degree (However, it may be only helpful when the input size is more larger than 320 * 240).
* ***trans***
    * Optional parameter. *Default: 0.3*.
    * Initial value of transmission.
    * The larger the initial value, the stronger the effect of dehazing, but the contrast may be too high.
* ***gamma***
    * Optional parameter. *Default: 1.5*.
    * Increase brightness to avoid image darkening after dehazing.
* ***air_size***
    * Optional parameter. *Default: 200*.
    * Block size in airlight estimation.
* ***trans_size***
    * Optional parameter. *Default: 16*.
    * Block size in transmission estimation.
* ***guide_size***
    * Optional parameter. *Default: 40*.
    * Block size in guide filter.
* ***post***
    * Optional parameter. *Default: False*.
    * Whether to post-process.
* ***lamda***
    * Optional parameter. *Default: 5.0*.
    * Empirical parameter for calculating pixel out-of-bounds loss. Generally do not need to be modified.

## Usage

Recommended to set small size ref clip.

```python
src = ...
ref = core.resize.Spline36(src, 320, 240)
res = core.dhce.DehazingCE(src, ref, trans=..., gamma=..., ...)
```

## Example

| Before dehazing | After dehazing |
| :-------------: | :------------: |
| <img width="360" src="https://i.loli.net/2020/06/13/h5jZJoc4KtSeuRn.jpg"> | <img width="360" src="https://i.loli.net/2020/07/30/2bXcZkMaIsy3rzm.jpg"> |
| <img width="360" src="https://i.loli.net/2020/06/12/rnjvJQdM6a3BZIg.jpg"> | <img width="360" src="https://i.loli.net/2020/06/12/hqgX9veIykwiL1r.jpg"> |

*The first image is from original paper. (Dehazing parameters: ref size: 320 \* 240, trans=0.35, gamma=1/0.65)*

*The second image is from [Wikipedia](https://en.wikipedia.org/wiki/File:20080313_Foggy_Street.jpg). (Dehazing parameters: ref size: same with input, trans=0.3, gamma=1/0.7)*

## Build

### Windows

Default VapourSynth include path is `C:/Program Files/VapourSynth/sdk/include`, if not, set with `-DVAPOURSYNTH_INCLUDE_DIR`.

```shell
mkdir build && cd build
cmake -G "NMake Makefiles" ..
cmake --build .
```

### Linux

Default VapourSynth include path is `/usr/local/include`, if not, set with `-DVAPOURSYNTH_INCLUDE_DIR`.

```shell
mkdir build && cd build
cmake ..
cmake --build .
```

### Windows and Linux using Github Actions

1.[Fork this repository](https://github.com/Kiyamou/VapourSynth-DehazingCE/fork).

2.Enable Github Actions on your fork: **Settings** tab -> **Actions** -> **General** -> **Allow all actions and reusable workflows** -> **Save** button.

3.Edit (if necessary) the file `.github/workflows/CI.yml` on your fork modifying the environment variable VapourSynth version:

```
env:
  VAPOURSYNTH_VERSION: <SET_YOUR_VERSION>
```

4.Go to the GitHub **Actions** tab on your fork, select **CI** workflow and press the **Run workflow** button (if you modified the `.github/workflows/CI.yml` file, a workflow will be already running and no need to run a new one).

When the workflow is completed you will be able to download the artifacts generated (Windows and Linux versions) from the run.

## Download Nightly Builds

**GitHub Actions Artifacts ONLY can be downloaded by GitHub logged users.**

Nightly builds are built automatically by GitHub Actions (GitHub's integrated CI/CD tool) every time a new commit is pushed to the _master_ branch or a pull request is created.

To download the latest nightly build, go to the GitHub [Actions](https://github.com/Kiyamou/VapourSynth-DehazingCE/actions/workflows/CI.yml) tab, enter the last run of workflow **CI**, and download the artifacts generated (Windows and Linux versions) from the run.

## License

[License](https://github.com/Kiyamou/VapourSynth-DehazingCE/blob/master/LICENSE) is from original source code.
