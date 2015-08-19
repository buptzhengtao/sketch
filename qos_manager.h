#ifdef NET_SELECT
#ifndef _QOS_MANAGER_H_
#define _QOS_MANAGER_H_

#include <winsock2.h>
#include <qos2.h>
#include <mutex>

#include "common.h"

class DLLEXP QOSManager
{
public:
    static QOSManager* instance();

    void add(SOCKET fd);
    void rmv(SOCKET fd);

private:
    QOSManager();
    ~QOSManager();

    QOSManager(const QOSManager&);
    QOSManager& operator=(const QOSManager&);

    QOS_FLOWID _flow_id;
    HANDLE _qos_handle;
    std::mutex _mutex;

    static QOSManager _instance;
    static std::mutex _instance_mutex;
};

#endif //_QOS_MANAGER_H_
#endif // NET_SELECT
