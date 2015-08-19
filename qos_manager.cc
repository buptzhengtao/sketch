#include "netio/select/qos_manager.h"

QOSManager QOSManager::_instance = NULL;
std::mutex QOSManager::_instance_mutex;

QOSManager* QOSManager::instance()
{
    if ( _instance == NULL ) {
        std::lock_guard<std::mutex> guard(_instance_mutex);

        if ( _instance == NULL ) {
            _instance = new QOSManager();
        }
    }

    return &_instance;
}

QOSManager::QOSManager() :
    _flow_id(0),
    _qos_handle(NULL)
{
    QOS_VERSION version;
    version.MajorVersion = 1;
    version.MinorVersion = 0;

    if ( !QOSCreateHandle(&version, &_qos_handle) ) {
        _qos_handle = NULL;
        DEBUG("Failed to create QOS handler, err(%d)", GetLastError());
    }
}

QOSManager::~QOSManager()
{
    if ( _qos_handle == NULL ) {
        return;
    }

    if ( _flow_id != 0 ) {
        QOSRemoveSocketFromFlow(_qos_handle, NULL, _flow_id, 0);
    }

    QOSCloseHandle(_qos_handle);
}

void QOSManager::add(SOCKET fd)
{
    std::lock_guard<std::mutex> guard(_mutex);

    if ( _qos_handle == NULL ) {
        return;
    }

    BOOL ret = QOSAddSocketToFlow(
        _qos_handle, 
        fd,
        NULL,
        QOSTrafficTypeVoice, 
        QOS_NON_ADAPTIVE_FLOW, 
        &_flow_id);

    if ( !ret ) {
        DEBUG("Failed to add socket to QOS, err(%d)", GetLastError());
    }
}

void QOSManager::rmv(SOCKET fd)
{
    std::lock_guard<std::mutex> guard(_mutex);

    if ( _qos_handle == NULL || _flow_id == 0 ) {
        return;
    }

    QOSRemoveSocketFromFlow(_qos_handle, fd, _flow_id, 0);
}

