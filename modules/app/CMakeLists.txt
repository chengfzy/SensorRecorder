# application

# recorder for MYNY-EYE camera
if(${WithMyntEyeD})
    add_subdirectory(MyntSimpleRecorder)
    add_subdirectory(MyntSensorRecorder)
endif()

# recorder for ZED camera(Open Capture library)
if(${WithZedOpen})
    add_subdirectory(ZedSimpleRecorder)
    add_subdirectory(ZedSensorRecorder)
endif()