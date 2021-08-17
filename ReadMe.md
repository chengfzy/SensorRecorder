# Sensor Recorder
> Sensor Recorder for Camera and IMU

## Introduce
This application is used for recorder camera and IMU for sensors, save to file and analysis it. Then main feature includes:
1. Provide an interface for data definition and recorder.
1. Support multiple camera device, include
    - [MYNY-EYE Depth](https://mynt-eye-d-sdk.readthedocs.io/zh_CN/latest/index.html)
    - [ZED Mini](https://www.stereolabs.com/zed-mini/)
1. Save file in `kalibr` format
1. Using `libjpeg-turbo` to compress image and save image int .jpg file
1. Analysis if some frame is lost using `scripts/analysis/data_checker.py`

Most of the codes are extracted from my own project `libra`(a framework for SLAM an 3D vision)

## Contact
chengcheng0829@gmail.com