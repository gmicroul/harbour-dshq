QT += quick network websockets dbus

CONFIG += c++11

# mapplauncherd boosters dlopen the binary when launched unsandboxed;
# that requires a position independent executable with `main` exported.
QMAKE_CXXFLAGS += -fPIE
QMAKE_LFLAGS += -pie -rdynamic

TARGET = harbour-dshq

SOURCES += \
    src/harbour-dshq.cpp \
    src/chatmodel.cpp \
    src/dshclient.cpp \
    src/processrunner.cpp \
    src/sessionmodel.cpp \
    src/systemdcontrol.cpp

HEADERS += \
    src/chatmodel.h \
    src/dshclient.h \
    src/processrunner.h \
    src/sessionmodel.h \
    src/systemdcontrol.h

OTHER_FILES += \
    qml/harbour-dshq.qml \
    qml/FishBackground.qml \
    qml/images/zhengmian.png \
    qml/cover/CoverPage.qml \
    qml/pages/ServicePage.qml \
    qml/pages/ChatPage.qml \
    qml/pages/ModelPickerPage.qml \
    rpm/harbour-dshq.yaml \
    rpm/harbour-dshq.spec
