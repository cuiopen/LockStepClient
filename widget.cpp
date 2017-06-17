#include "widget.h"
#include "ui_widget.h"
#include <iostream>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    init();
    initCommand();
}

Widget::~Widget()
{
    delete ui;
}

//逻辑帧 100ms
void Widget::timeoutUpdate()
{
    //帧同步 掉线之后，无法加入，
    //因为 帧同步记录的是玩家的操作， 所以掉线之后，当前帧的ID已经改变了， 无法知道其他玩家当前的状态； 除非把所有玩家的操作全部保存下来， 等玩家重连上来， 全部发过去;
    update();
}

void Widget::logicFrameRefresh(struct FrameInfo& frameInfo)
{
    //klog_info("curId : %d, netId : %d", curFrameId, frameInfo.curFrameId);
    //std::cout << "curId : " << curFrameId << "frameInfo : " << frameInfo.curFrameId << std::endl;
    if ( curFrameId == frameInfo.curFrameId )
    {
        //触发上报逻辑
        updateKeyInfoToServer();
        //得到下一帧的ID
        //nextFrameId = it->second.nextFrameId;
        //执行操作的逻辑, 根据操作更新每个玩家信息
        runKeyInfo(frameInfo.mUidFrameInfo);
        //curFrameId = frameInfo.nextFrameId;
    }
}

void Widget::runKeyInfo(std::map<uint32, VeFrameInfo>& mOpInfo)
{
    // 逻辑帧运行
    int step = 5;
    //std::cout << "mOpInfo : " << mOpInfo.size() << std::endl;
    for (auto uValue : mOpInfo) {
        uint32 uid = uValue.first;
        auto player = mAllPlayer.find(uid);
        if (player == mAllPlayer.end()) continue;

        for (auto fValue : uValue.second) {
            std::cout << "fValue : " << fValue << std::endl;
            switch(fValue) {
                case E_PLAYER_KEY_UP:
                    player->second.y -= step;
                    break;
                case E_PLAYER_KEY_DOWN:
                    player->second.y += step;
                    break;
                case E_PLAYER_KEY_LEFT:
                    player->second.x -= step;
                    break;
                case E_PLAYER_KEY_RIGHT:
                    player->second.x += step;
                    break;
            }
            //std::cout << "update : " << std::endl;
            update();
        }
    }
}

void Widget::timeoutHeartbeat()
{
    if ( tcpSocket->state() != QAbstractSocket::ConnectedState ) return;
    cs::C2S_Ping ping;

    QDateTime time = QDateTime::currentDateTime();
    QTime current_time = QTime::currentTime();
    uint32 timems = current_time.msec();

    ping.set_send_time(timems);

    std::string msg;
    msg = ping.SerializeAsString();
    sendPacket(uint32(cs::ProtoID::ID_C2S_Ping), msg);
}

void Widget::initCommand()
{
    REGISTER_CMD_CALLBACK(cs::ID_S2C_Login, handlerLogin);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_Move, handlerMove);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_AllPos, handlerUpdateAllUsers);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_Ping, handlerHeartbeat);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_Frame, handlerFrameRefresh);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_FrameInit, handlerFrameInit);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_Ready, handlerReady);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_Sight, handlerSight);
}

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    // 反走样
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 设置画笔颜色、宽度
    painter.setPen(QPen(QColor(0, 160, 230), 2));

    painter.drawRect(QRect(mapxBegin,mapyBegin, mapxEnd, mapyEnd));
    QBrush brush(Qt::black);
    painter.setBrush(brush);

    for (auto item : mAllPlayer)
    {
        painter.drawEllipse(QPointF(item.second.x, item.second.y), 5, 5);
    }

   static QTime time;
   static int frames = 0;
   static bool started = false;

   if (!started || time.elapsed() > 1000)
   {
       qreal fps = frames * 1000. / time.elapsed();
       if (fps == 0)
           m_current_fps = "fps ";
       else
           m_current_fps = QString::fromLatin1("fps %3").arg((int) qRound(fps));

       time.start();
       started = true;
       frames = 0;
   }
   else
   {
       ++frames;
       painter.setPen(QPen(QColor(52, 233, 48), 2));
       painter.setFont(QFont("times", 14));
       painter.drawText(width() - 100, 30, m_current_fps);
       //sklog_info("m_current_fps : %s", m_current_fps.toStdString().c_str());

       painter.setFont(QFont("rtt", 14));
       painter.drawText(width() - 100, 50, m_current_rtt);
   }

}

void Widget::mousePressEvent(QMouseEvent *e)
{

}

void Widget::keyInfoOp(uint32 dwop)
{
    if ( tcpSocket->state() != QAbstractSocket::ConnectedState ) return;

    std::unique_lock<std::mutex> lck(mtx);
    vCurFrameInfo.push_back(dwop);
    //klog_info("keyinfo >>>> size : %d", vCurFrameInfo.size());
}

void Widget::keyPressEvent(QKeyEvent *e)
{
    int step = 5;

    switch (e->key())
    {
        case Qt::Key_W :
        {
            //y = y-step; playerMove(x, y);
            keyInfoOp(E_PLAYER_KEY_UP);
            break;
        }
        case Qt::Key_S :
        {
            //y = y+step; playerMove(x, y);
            keyInfoOp(E_PLAYER_KEY_DOWN);
            break;
        }
        case Qt::Key_A :
        {
            //x = x-step; playerMove(x, y);
            keyInfoOp(E_PLAYER_KEY_LEFT);
            break;
        }
        case Qt::Key_D :
        {
            //x = x+step; playerMove(x, y);
            keyInfoOp(E_PLAYER_KEY_RIGHT);
            break;
        }
        //connect server
        case Qt::Key_Space : connectServer(); break;
        case Qt::Key_R: playerReady(); break;
    }

}

void Widget::init()
{
    /////// map is 500 * 500
    mapxBegin = 10;
    mapyBegin = 10;
    mapxEnd = mapxBegin + 500;
    mapyEnd = mapyBegin + 500;
    dwUid = 0;
    this->setFixedSize(mapxEnd + 20, mapyEnd + 20);
    /////////network/////////
    this->tcpSocket = new QTcpSocket(this);

    connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(revData()));

    connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),
               this,SLOT(displayError(QAbstractSocket::SocketError)));

    ////////heartbeat/////////
    m_current_rtt = "rtt -1ms";
    m_heartbeat = new QTimer;
    connect(m_heartbeat,SIGNAL(timeout()),this,SLOT(timeoutHeartbeat()));
    m_heartbeat->start(10000);//10s 心跳

    ////////渲染帧////////////
    m_timer = new QTimer;
    connect(m_timer,SIGNAL(timeout()),this,SLOT(timeoutUpdate()));
    m_timer->start(16); //60帧

    /////////frame//////// 1s  50  20 * 5= 100 -> 10
//    sumFrameAdd = 0;
//    upStep = 5;
//    curFrameId = sumFrameAdd + upStep;
}

void Widget::connectServer()
{
    tcpSocket->abort();
    tcpSocket->connectToHost("192.168.119.129",2007);
    klog_info("connect server");
    playerLogin();
}

void Widget::revData()
{
   QByteArray datas = tcpSocket->readAll();

   buf_.append(datas.data(), datas.size());

   for (;;)
   {
       if ( buf_.size() < PACKET_HEAD_LEN )
       {
           break;
       }

       uint32 msgLen = buf_.peekUint32();
       if ( msgLen > buf_.size() ) {
         klog_info(">>>>>>> MSG LEN : %d | buf len : %d", msgLen, buf_.size());
         break;
       }

       struct PACKET pkt;
       pkt.len = buf_.readInt32();
       pkt.cmd = buf_.readInt32();
       pkt.uid = buf_.readInt32();
       pkt.msg = std::string(buf_.retrieveBuf(pkt.len - PACKET_HEAD_LEN));

       if ( command_.find(pkt.cmd) != command_.end() ) {
            command_[pkt.cmd](pkt.msg);
       }
   }
   //klog_info("BUFFER size : %d", buf_.size());
   //klog_info("Proto Callback Not Found This Func : %d", pkt.cmd);
}


void Widget::displayError(QAbstractSocket::SocketError)
{
   qDebug()<<tcpSocket->errorString();

   tcpSocket->close();
}

void Widget::sendPacket(int cmdId, std::string& msg)
{
    struct PACKET pkt;

    pkt.len = PACKET_HEAD_LEN + msg.size();
    pkt.cmd = cmdId;
    pkt.uid = this->dwUid;
    pkt.msg = msg;

    char* msg_ = new char[pkt.len];
    memset(msg_, 0, pkt.len);
    memcpy(msg_, (char*)&(pkt.len), sizeof(uint32));
    memcpy(msg_ + 4, (char*)&pkt.cmd, sizeof(uint32));
    memcpy(msg_ + 8, (char*)&pkt.uid, sizeof(uint32));
    memcpy(msg_ + 12, pkt.msg.c_str(), msg.size());

    tcpSocket->write(msg_, pkt.len);
    //klog_info("cmdId : %d | msg len : %d | sum : %d", pkt.cmd, msg.size(), pkt.len);
    delete msg_;
}

void Widget::updateKeyInfoToServer()
{
    cs::C2S_Frame frame;
    uint32* uFrame;
    frame.set_uid(dwUid);
    frame.set_frameid(curFrameId);

    std::unique_lock<std::mutex> lck(mtx);
    for (std::vector<uint32>::iterator it = vCurFrameInfo.begin(); it != vCurFrameInfo.end(); ++it )
    {
        frame.add_key_info(*it);
    }
    vCurFrameInfo.clear();

    std::string msg;
    msg = frame.SerializeAsString();
    sendPacket(uint32(cs::ProtoID::ID_C2S_Frame), msg);
}

/////////////////////////
void Widget::playerLogin()
{
    cs::C2S_Login login;

    std::string msg;
    msg = login.SerializeAsString();
    sendPacket(uint32(cs::ProtoID::ID_C2S_Login), msg);
}

void Widget::playerMove(int x, int y)
{
    cs::C2S_Move move;
    move.set_dwx(x);
    move.set_dwy(y);

    std::string msg;
    msg = move.SerializeAsString();
    sendPacket(uint32(cs::ProtoID::ID_C2S_Move), msg);
}

void Widget::playerAttack()
{

}

void Widget::playerReady()
{
    cs::C2S_Ready ready;

    std::string msg;
    msg = ready.SerializeAsString();
    sendPacket(uint32(cs::ProtoID::ID_C2S_Ready), msg);
}


////////////////////
bool Widget::handlerLogin(std::string& str)
{
    cs::S2C_Login login;
    if ( !login.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }
    if (dwUid != 0)
    {
        klog_info("invalid handlerLogin >>>> dwUid : %d", dwUid);
        return false;
    }
    dwUid = login.uid();
    struct Player player(mapxBegin,mapyBegin, login.uid());

    mAllPlayer.insert(std::make_pair(login.uid(), player));

    return true;
}

bool Widget::handlerMove(std::string& str)
{
    cs::S2C_Move move;
    if ( !move.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    uint32 uid = move.dwuid();
    uint32 dwx = move.dwx();
    uint32 dwy = move.dwy();

    // 如果是自己, 则更新自己即可
    // if ( play.uid == uid )
    // {
    //     play.x = dwx;
    //     play.y = dwy;
    //     update();
    //     //klog_info("Self Change Pos %d %d", play.x, play.y);
    // }

    return true;
}

bool Widget::handlerUpdateAllUsers(std::string& str)
{
    cs::S2C_AllPos pos;
    if ( !pos.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    int size = pos.users_size();
    for (int i = 0; i < size; i++)
    {
        cs::User user = pos.users(i);
        struct Player p;
        p.x = user.dwx();
        p.y = user.dwy();
        p.uid = user.uid();

        struct Player& otherPlay = mAllPlayer.insert(std::make_pair(p.uid, p)).first->second;
        otherPlay.x = p.x;
        otherPlay.y = p.y;
    }

    update();
    //klog_info(" sight : %d", play.mSights.size());
    return true;
}

bool Widget::handlerHeartbeat(std::string& str)
{
    cs::S2C_Ping sping;

    if ( !sping.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    QDateTime time = QDateTime::currentDateTime();
    QTime current_time = QTime::currentTime();
    uint32 timems = current_time.msec();

    uint32 sendtime = sping.send_time();
    qreal rtt = qreal(timems - sendtime);

    m_current_rtt = QString::fromLatin1("rtt %3 ms").arg((int) qRound(rtt));
}

bool Widget::handlerFrameInit(std::string& str)
{
    cs::S2C_FrameInit frameinit;

    if ( !frameinit.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    curFrameId = frameinit.cur_frame_id();
    nextFrameId = frameinit.next_frame_id();
    klog_info("handler FrameInit");
}

bool Widget::handlerFrameRefresh(std::string& str)
{
    cs::S2C_Frame frame;

    if ( !frame.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    struct FrameInfo frameInfo;
    frameInfo.curFrameId = frame.frame_id();
    frameInfo.nextFrameId = frame.nextframe_id();
    //klog_info("handlerFrameRefresh : c : %d, n : %d", frame.frame_id(), frame.nextframe_id());
    int size = frame.users_size();
    for (int i = 0; i < size; i++)
    {
        cs::UserFrame user = frame.users(i);

        VeFrameInfo p;
        for (int j = 0; j < user.key_info_size(); j++)
        {
            uint32 key = user.key_info(j);
            p.push_back(key);
        }
        frameInfo.mUidFrameInfo.insert(std::make_pair(user.uid(), p));
    }

    logicFrameRefresh(frameInfo);
    curFrameId = frame.nextframe_id();
}

bool Widget::handlerReady(std::string& str)
{
    cs::S2C_Ready sready;

    if ( !sready.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }
}

bool Widget::handlerSight(std::string& str)
{
    cs::S2C_Sight ssight;

    if ( !ssight.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    klog_info("handlerSight");

    int size = ssight.users_size();
    for (int i = 0; i < size; i++)
    {
        cs::User user = ssight.users(i);
        struct Player player(user.dwx(), user.dwy(), user.uid());
        mAllPlayer.insert(std::make_pair(user.uid(), player));
    }
}
