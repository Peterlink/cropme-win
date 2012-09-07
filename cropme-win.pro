QT += gui network

#CONFIG += console

HEADERS = screenshot.h
SOURCES = main.cpp\
            screenshot.cpp

win32 {
    RC_FILE += icon.rc
    OTHER_FILES += icon.rc
}
