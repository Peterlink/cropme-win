QT += gui network

HEADERS = screenshot.h \
    logwriter.h \
    screenmanager.h
SOURCES = main.cpp\
            screenshot.cpp \
    logwriter.cpp \
    screenmanager.cpp

win32 {
    RC_FILE += icon.rc
    OTHER_FILES += icon.rc
}
