#ifdef ASSERT
#undef ASSERT
#endif

#include <qos2.h>

#include "netio/select/qos_manager.h"

typedef BOOL (*QOSCreateHandleFunc)(QOS_VERSION*, HANDLE*);

typedef BOOL (*QOSAddSocketToFlowFunc)(HANDLE, 
                                    SOCKET,
                                    sockaddr*,
                                    QOS_TRAFFIC_TYPE,
                                    DWORD,
                                    ULONG*);

typedef BOOL (*QOSRemoveSocketFromFlowFunc)(HANDLE, 
                                         SOCKET,
                                         ULONG,
                                         DWORD);

typedef BOOL (*QOSCloseHandleFunc)(HANDLE);

static HINSTANCE QOSdllHandle = NULL;
static QOSCreateHandleFunc _QOSCreateHandle = NULL;
static QOSAddSocketToFlowFunc _QOSAddSocketToFlow = NULL;
static QOSRemoveSocketFromFlowFunc _QOSRemoveSocketFromFlow = NULL;
static QOSCloseHandleFunc _QOSCloseHandle = NULL;

static int LoadLibQwave()
{
    QOSdllHandle = LoadLibrary("qwave.dll");

    if ( QOSdllHandle == NULL ) {
        return -1;
    }

    _QOSCreateHandle = 
        (QOSCreateHandleFunc)GetProcAddress(
            QOSdllHandle, 
            "QOSCreateHandle");

    _QOSAddSocketToFlow =
        (QOSAddSocketToFlowFunc)GetProcAddress(
            QOSdllHandle, 
            "QOSAddSocketToFlow");

    _QOSRemoveSocketFromFlow = 
        (QOSRemoveSocketFromFlowFunc)GetProcAddress(
            QOSdllHandle,
            "QOSRemoveSocketFromFlow");

    _QOSCloseHandle =
        (QOSCloseHandleFunc)GetProcAddress(
            QOSdllHandle,
            "QOSCloseHandle");

    if ( _QOSCreateHandle == NULL ||
         _QOSAddSocketToFlow == NULL ||
         _QOSRemoveSocketFromFlow == NULL ||
         _QOSCloseHandle == NULL ) {
        return -1;
    }

    return 0;
}

statuc void FreeLibQwave()
{
    if ( QOSdllHandle != NULL ) {
        FreeLibrary(QOSdllHandle);
    }
}

QOSManager* QOSManager::_instance = NULL;
std::mutex QOSManager::_instance_mutex;

QOSManager* QOSManager::instance()
{
    if ( _instance == NULL ) {
        std::lock_guard<std::mutex> guard(_instance_mutex);

        if ( _instance == NULL ) {
            _instance = new QOSManager();
        }
    }

    return _instance;
}

QOSManager::QOSManager() :
    _flow_id(0),
    _qos_handle(NULL)
{
    if ( LoadLibQwave() != 0 ) {
        return;
    }

    QOS_VERSION version;
    version.MajorVersion = 1;
    version.MinorVersion = 0;

    if ( !_QOSCreateHandle(&version, &_qos_handle) ) {
        _qos_handle = NULL;
        //DEBUG("Failed to create QOS handler, err(%d)", GetLastError());
    }
}

QOSManager::~QOSManager()
{
    if ( _qos_handle != NULL ) {
        if ( _flow_id != 0 ) {
            _QOSRemoveSocketFromFlow(_qos_handle, NULL, _flow_id, 0);
        }

        _QOSCloseHandle(_qos_handle);
    }

    FreeLibQwave();
}

void QOSManager::add(SOCKET fd)
{
    std::lock_guard<std::mutex> guard(_mutex);

    if ( _qos_handle == NULL ) {
        return;
    }

    BOOL ret = _QOSAddSocketToFlow(
        _qos_handle, 
        fd,
        NULL,
        QOSTrafficTypeVoice, 
        QOS_NON_ADAPTIVE_FLOW, 
        &_flow_id);

    if ( !ret ) {
        //DEBUG("Failed to add socket to QOS, err(%d)", GetLastError());
    }
}

void QOSManager::rmv(SOCKET fd)
{
    std::lock_guard<std::mutex> guard(_mutex);

    if ( _qos_handle == NULL || _flow_id == 0 ) {
        return;
    }

    _QOSRemoveSocketFromFlow(_qos_handle, fd, _flow_id, 0);
}

