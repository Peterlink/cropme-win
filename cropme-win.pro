QT += gui network

HEADERS = screenshot.h \
    logwriter.h
SOURCES = main.cpp\
            screenshot.cpp \
    logwriter.cpp

win32 {
    RC_FILE += icon.rc
    OTHER_FILES += icon.rc
}
