#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include<QtNetwork>
#include<QtNetwork/QTcpSocket>
#include<QtNetwork/QTcpServer>

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTime>
#include <string>
#include <map>
#include <functional>
#include <mutex>
#include <QDateTime>

#include "proto/cs.pb.h"
#include "buffer.h"
#include "log.h"

namespace Ui {
class Widget;
}

#define REGISTER_CMD_CALLBACK(cmdId, func) \
    command_[uint32(cmdId)]  = std::bind(&Widget::func, this, std::placeholders::_1)


const uint32 PACKET_HEAD_LEN = 12;
struct PACKET {
    PACKET() : len(0), cmd(0), uid(0) {}
    uint32 len;
    uint32 cmd;
    uint32 uid;
    std::string msg;
};

struct Player {
    Player():x(0), y(0), uid(0) {}
    int x;
    int y;
    int uid;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    enum {
        E_PLAYER_KEY_UP    = 1,
        E_PLAYER_KEY_DOWN  = 2,
        E_PLAYER_KEY_LEFT  = 3,
        E_PLAYER_KEY_RIGHT = 4,
    };
    typedef std::vector<uint32> VeFrameInfo;
    typedef std::map<uint64, FrameInfo> MapServerFrameInfo;
    typedef std::function<bool(std::string&)> ServiceFunc;
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    void init();
    void connectServer();
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *);

    ///////update////////
    /// \brief timeoutUpdate
private slots:
    void timeoutUpdate();
    void timeoutHeartbeat();

public:
    ///////network///////
    void initCommand();
    void sendPacket(int, std::string&);

    void updateKeyInfoToServer();
    void runKeyInfo(std::map<uint32, VeFrameInfo>& mOpInfo);
    void logicFrameRefresh(struct FrameInfo& frameInfo);
    ///////
    void playerLogin();
    void playerMove(int, int);
    void playerAttack();

    ///////////
    bool handlerUpdateAllUsers(std::string& str);
    bool handlerLogin(std::string&);
    bool handlerMove(std::string&);
    bool handlerHeartbeat(std::string&);
    bool handlerFrameRefresh(std::string&);
    bool handlerFrameInit(std::string&);

private:
    int mapxBegin;
    int mapyBegin;
    int mapxEnd;
    int mapyEnd;

    /////////
    Buffer buf_;
    std::map<uint32, struct Player> mAllPlayer;
    std::map<uint32, ServiceFunc> command_;
    QString m_current_fps;
    QString m_current_rtt;
    QTimer* m_timer;
    /////
    /// \brief m_heartbeat
    ///
    uint32 sendTime;
    QTimer* m_heartbeat;

    uint32 upStep;
    uint64 curFrameId;
    uint64 nextFrameId;

    struct FrameInfo {
        uint64 curFrameId;
        std::map<uint32, VeFrameInfo> mUidFrameInfo;
    };

    MapServerFrameInfo mFrameInfo;
    ///////////
    VeFrameInfo vCurFrameInfo;
    std::mutex mtx;
    ///////////

private slots:
     void revData();
     void displayError(QAbstractSocket::SocketError);
private:
    QTcpSocket *tcpSocket;
    Ui::Widget *ui;
};

#endif // WIDGET_H
